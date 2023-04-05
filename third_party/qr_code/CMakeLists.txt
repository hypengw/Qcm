add_library(qr_code STATIC include/qr_code/qrcodegen.hpp qrcodegen.cpp)

target_include_directories(
  qr_code
  PUBLIC include
  PRIVATE include/qr_code)
