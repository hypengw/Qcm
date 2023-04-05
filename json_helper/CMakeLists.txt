add_library(json_helper STATIC include/json_helper/helper.h
                               include/json_helper/helper.inl helper.cpp)

target_include_directories(
  json_helper
  PUBLIC include
  PRIVATE include/json_helper .)

target_link_libraries(json_helper PUBLIC core nlohmann_json::nlohmann_json)
