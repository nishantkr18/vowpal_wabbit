#pragma once
 
#include <thread>
#include <future>
#include <mutex> 
#include <chrono>  
 
//Adds all lines of input to the input queue. The function version on this branch (multithread_parser_with_passes) supports passes.
inline void io_lines_toqueue(vw& all){

  std::mutex mut;
  std::unique_lock<std::mutex> lock(mut);
  
  int num_passes_to_complete = all.numpasses;
 
  int counter_of_number_passes = 0;
  while(counter_of_number_passes++ < num_passes_to_complete) {

    //  zeset done_with_io (set when we are done adding to io) to false at beginning of new pass
    all.example_parser->done_with_io.store(false);

    char* line = nullptr;

    bool should_finish = false;

    while(!should_finish)
    {   
      should_finish = all.example_parser->input_file_reader(all, line);
    }

    while(all.example_parser->done_with_io == false) {
      all.example_parser->can_end_pass.wait(lock);
    }

    if (counter_of_number_passes != num_passes_to_complete && num_passes_to_complete > 1) {
      reset_source(all, all.num_bits);
      all.do_reset_source = false;

      // increment current pass in io thread
      all.current_pass++;
    }

  }

  all.example_parser->io_lines.set_done();

 
}
 

inline bool read_input_file_ascii(vw& all, char *&line) {
 
 size_t num_chars_initial = all.example_parser->input->readto(line, '\n');
 
 bool finish = false;
 
  std::vector<char> *byte_array = new std::vector<char>();
  byte_array->resize(num_chars_initial); // Note: This byte_array is NOT null terminated!

  memcpy(byte_array->data(), line, num_chars_initial);

  all.example_parser->io_lines.push(std::move(byte_array));
 
  if(num_chars_initial <= 0){
    finish = true;
  }
 
 return finish;
 
}
 
inline bool read_input_file_binary(vw& all, char *&line) {
 
 size_t total_num_input_bytes = 0;
 
 //read_all_data returns the number of bytes successfully read from the input files
 total_num_input_bytes = all.example_parser->input->read_all_data(line, total_num_input_bytes);
 
 bool finish = true;
 
  std::vector<char> *byte_array = new std::vector<char>();
  byte_array->resize(total_num_input_bytes); // Note: This byte_array is NOT null terminated!

  memcpy(byte_array->data(), line, total_num_input_bytes);

  all.example_parser->io_lines.push(std::move(byte_array));
 
 return finish;

}
 
//try to encapsulate start io thread and end io thread, so it can be moved out of parse dispatch loop.
namespace VW {
 
inline void start_io_thread(vw& all){
 
 if(!all.early_terminate) {
   all.io_thread = std::thread([&all]()
   {
     io_lines_toqueue(all);
   });
 }
 
}
 
inline void end_io_thread(vw& all){
 
 all.io_thread.join();
 
}
 
}
