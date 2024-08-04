add_executable(
  qcm_model_generator
  peg_cpp.hpp generator.cpp)
target_include_directories(
  qcm_model_generator
  PRIVATE .)
target_link_libraries(
  qcm_model_generator
  PRIVATE core pegtl Qt::Core)