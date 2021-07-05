// Copyright (c) by respective owners including Yahoo!, Microsoft, and
// individual contributors. All rights reserved. Released under a BSD (revised)
// license as described in the file LICENSE.
#pragma once
#include "io_buf.h"
#include "example.h"
#include "../explore/future_compat.h"

// Mutex and CV cannot be used in managed C++, tell the compiler that this is unmanaged even if included in a managed
// project.
#ifdef _M_CEE
#  pragma managed(push, off)
#  undef _M_CEE
#  include <mutex>
#  include <condition_variable>
#  define _M_CEE 001
#  pragma managed(pop)
#else
#  include <mutex>
#  include <condition_variable>
#endif

#include <atomic>
#include <memory>
#include "vw_string_view.h"
#include "queue.h"
#include "object_pool.h"
#include "hashstring.h"
#include "simple_label_parser.h"

struct vw;
struct input_options;
struct parser
{
  parser(size_t ring_size, bool strict_parse_, int num_parse_threads)
      : example_pool{ring_size}
      , example_vector_pool{ring_size}
      , ready_parsed_examples{ring_size}
      , ring_size{ring_size}
      , begin_parsed_examples(0)
      , end_parsed_examples(0)
      , finished_examples(0)
      , num_parse_threads{num_parse_threads}
      , strict_parse{strict_parse_}
      , io_lines{100000}
  {
    this->input = VW::make_unique<io_buf>();
    this->output = VW::make_unique<io_buf>();
    this->lbl_parser = simple_label_parser;
  }

  // delete copy constructor
  parser(const parser&) = delete;
  parser& operator=(const parser&) = delete;

  // helper(s) for text parsing
  std::vector<VW::string_view> words;

  VW::object_pool<example> example_pool;
  VW::object_pool<std::vector<example*>> example_vector_pool;
  VW::ptr_queue<std::vector<example*>> ready_parsed_examples;

  std::unique_ptr<io_buf> input;  // Input source(s)
  /// reader consumes the input io_buf in the vw object and is generally for file based parsing
  int (*reader)(vw*, std::vector<example*>& examples, std::vector<VW::string_view>& words, std::vector<VW::string_view>& parse_name, std::vector<char>* io_lines_next_item);
  /// text_reader consumes the char* input and is for text based parsing
  void (*text_reader)(vw*, const char*, size_t, std::vector<example*>&);

  bool (*input_file_reader)(vw& vw, char*& line);

  shared_data* _shared_data = nullptr;

  hash_func_t hasher;
  bool resettable;           // Whether or not the input can be reset.
  std::unique_ptr<io_buf> output;  // Where to output the cache.
  std::string currentname;
  std::string finalname;

  bool write_cache = false;
  bool sort_features = false;
  bool sorted_cache = false;

  const size_t ring_size;
  std::atomic<uint64_t> begin_parsed_examples;  // The index of the beginning parsed example.
  std::atomic<uint64_t> end_parsed_examples;    // The index of the fully parsed example.
  std::atomic<uint64_t> finished_examples;      // The count of finished examples.
  uint32_t in_pass_counter = 0;
  bool emptylines_separate_examples = false;  // true if you want to have holdout computed on a per-block basis rather
                                              // than a per-line basis
  int num_parse_threads = 1; // The number of parse threads to use

  std::mutex output_lock;
  std::condition_variable output_done;

  //for multithreaded parsing
  //for reader function
  std::mutex parser_mutex;
  std::condition_variable example_parsed;
  //for cv notify and wait
  std::mutex example_cv_mutex;

  bool done = false;

  v_array<size_t> ids;     // unique ids for sources
  v_array<size_t> counts;  // partial examples received from sources
  size_t finished_count;   // the number of finished examples;
  int bound_sock = 0;

  std::vector<VW::string_view> parse_name;

  label_parser lbl_parser;  // moved from vw

  bool audit = false;
  bool decision_service_json = false;

  bool strict_parse;
  std::exception_ptr exc_ptr;

  VW::ptr_queue<std::vector<char>> io_lines;
  std::atomic<bool> done_with_io{false};
  // for passes
  std::condition_variable can_end_pass;

};

void enable_sources(vw& all, bool quiet, size_t passes, input_options& input_options);

VW_DEPRECATED("Function is no longer used")
void adjust_used_index(vw& all);

// parser control
void lock_done(parser& p);
void set_done(vw& all);

// source control functions
void reset_source(vw& all, size_t numbits);
VW_DEPRECATED("Function is no longer used")
void finalize_source(parser* source);
VW_DEPRECATED("Function is no longer used")
void set_compressed(parser* par);

void free_parser(vw& all);

void notify_examples_cv(example *& ex);
