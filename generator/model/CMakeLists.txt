cmake_minimum_required(VERSION 3.14)

project(
  QcmModelGenerator
  LANGUAGES CXX
)
find_package(
  Qt6 REQUIRED
  COMPONENTS Core)

if(0)
find_package(FLEX REQUIRED)
flex_target(CppScanner cpp_scanner/lexer.l ${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp)
endif()

add_executable(
  qcm_model_generator
  peg_cpp.hpp generator.cpp ${FLEX_CppScanner_OUTPUTS})
target_include_directories(
  qcm_model_generator
  PRIVATE . cpp_scanner)
target_link_libraries(
  qcm_model_generator
  PRIVATE core taocpp::pegtl Qt::Core)
