#include "example_predict.h"
#include "array_parameters.h"

#include "example_predict_builder.h"
#include "vw_slim_predict.h"

#include <emscripten/bind.h>

int main()
{
  return 0;
}

using namespace emscripten;

example_predict* new_example_predict()
{
  return new example_predict();
}

void destroy_example_predict(example_predict* ex)
{
  delete ex;
}

void push_feature_string(vw_slim::example_predict_builder& arr, const std::string& feature_idx, float value) {
    arr.push_feature_string((char*)feature_idx.c_str(), value);
}

void load_model(vw_slim::vw_predict<dense_parameters>& vw, const std::vector<char>& bytes) {
  vw.load(bytes.data(), bytes.size());
}

struct predict_results
{
  std::vector<float> pdf;
  std::vector<int> ranking;

  std::vector<float> get_pdf()
  {
    return pdf;
  }

  std::vector<int> get_ranking()
  {
    return ranking;
  }
};

struct action_list
{
  std::vector<example_predict*> actions;
  void add_action(example_predict* a)
  {
    actions.push_back(a);
  }
};

predict_results predict(vw_slim::vw_predict<dense_parameters>& vw, const std::string& event_id, example_predict* shared, const action_list& actions)
{
  predict_results results;
  vw.predict(event_id.c_str(), *shared, (example_predict*)actions.actions.data(), actions.actions.size(), results.pdf, results.ranking);
  return results;
}

EMSCRIPTEN_BINDINGS(vw) {

  class_<example_predict>("example_predict");

  function("new_example_predict", &new_example_predict, allow_raw_pointers());
  function("destroy_example_predict", &destroy_example_predict, allow_raw_pointers());

  class_<vw_slim::example_predict_builder>("example_predict_builder")
    .constructor<example_predict*, const std::string&>()
    .function("push_feature_string", &push_feature_string, allow_raw_pointers())
    .function("push_feature", &vw_slim::example_predict_builder::push_feature, allow_raw_pointers());

  class_<vw_slim::vw_predict<dense_parameters>>("vw_predict")
    .constructor<>()
    .function("load_model", &load_model)
    .function("predict", &predict, allow_raw_pointers());

  class_<predict_results>("predict_results")
    .function("get_pdf", &predict_results::get_pdf)
    .function("get_ranking", &predict_results::get_ranking);

  class_<action_list>("action_list")
    .constructor<>()
    .function("add_action", &action_list::add_action, allow_raw_pointers());

  register_vector<int>("VectorInt");
  register_vector<float>("VectorFloat");
};