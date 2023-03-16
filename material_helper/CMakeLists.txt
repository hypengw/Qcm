add_library(material_helper STATIC include/material_helper/helper.h helper.cpp)

target_include_directories(material_helper PUBLIC include)

target_link_libraries(
  material_helper
  PUBLIC Qt6::Core core
  PRIVATE material_color)
set_property(TARGET material_helper PROPERTY POSITION_INDEPENDENT_CODE ON)
