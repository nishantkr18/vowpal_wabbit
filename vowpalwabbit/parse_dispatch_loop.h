// Copyright (c) by respective owners including Yahoo!, Microsoft, and
// individual contributors. All rights reserved. Released under a BSD (revised)
// license as described in the file LICENSE.

#pragma once

#include <functional>
#include <thread>
#include <future>
#include <mutex>  
#include "io_to_queue.h"

#include "io/logger.h"

using dispatch_fptr = std::function<void(vw&, const std::vector<example*>&)>;
struct io_state;

inline void parse_dispatch(vw& all, dispatch_fptr dispatch)
{
  
  size_t example_number = 0;  // for variable-size batch learning algorithms

  // we need to allow reuse of vectors, instead of deleting and recreating them for each example.
  // used in substring_to_example.
  std::vector<VW::string_view> words_localcpy;
  std::vector<VW::string_view> parse_name_localcpy;

  try
  {
    while (!all.example_parser->done)
    {
      example* example_ptr = &VW::get_unused_example(&all);
      std::vector<example*>* examples = &VW::get_unused_example_vector(&all);
      (*examples).push_back(example_ptr);

      bool is_null_pointer = false;

      // This logic should be outside the reader, as:
      // 1. it is common to all types of parsers.
      // 2. the type of parser might change when the thread is waiting for a pop of io_lines.
      std::vector<char> *io_lines_next_item = nullptr;
      {
        std::lock_guard<std::mutex> lck(all.example_parser->parser_mutex);
        
        io_lines_next_item = all.example_parser->io_lines.pop();

        if(io_lines_next_item != nullptr) {
          all.example_parser->ready_parsed_examples.push(examples);
        }
      }


      if(io_lines_next_item == nullptr) {
        VW::finish_example(all, *example_ptr);
        VW::finish_example_vector(all, *examples);
      }

      else{
        int num_chars_read = all.example_parser->reader(&all, *examples, words_localcpy, parse_name_localcpy, io_lines_next_item);

        if(num_chars_read > 0){
          dispatch(all, *examples);
        }
        else if(num_chars_read == 0){
          example_ptr->end_pass = true;
          dispatch(all, *examples);
        }
      }     
      delete io_lines_next_item;


    }

  }
  
  catch (VW::vw_exception& e)
  {
    VW::io::logger::errlog_error("vw example #{0}({1}:{2}): {3}", example_number, e.Filename(), e.LineNumber(), e.what());

    // Stash the exception so it can be thrown on the main thread.
    all.example_parser->exc_ptr = std::current_exception();
  }
  catch (std::exception& e)
  {
    VW::io::logger::errlog_error("vw: example #{0}{1}", example_number, e.what());

    // Stash the exception so it can be thrown on the main thread.
    all.example_parser->exc_ptr = std::current_exception();
  }
  lock_done(*all.example_parser);
}
