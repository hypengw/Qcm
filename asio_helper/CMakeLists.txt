add_library(
  asio_helper SHARED
  include/asio_helper/helper.h include/asio_helper/sync_file.h
  include/asio_helper/watch_dog.h asio.cpp)
target_include_directories(
  asio_helper
  PUBLIC include)
target_compile_definitions(
  asio_helper
  PUBLIC ASIO_DYN_LINK ASIO_DISABLE_VISIBILITY)
target_link_libraries(
  asio_helper
  PUBLIC core asio)
set_target_properties(
  asio_helper
  PROPERTIES CXX_VISIBILITY_PRESET hidden
             OUTPUT_NAME qcm_asio)
install(
  TARGETS asio_helper
  BUNDLE DESTINATION .
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})