add_library(
  core STATIC
  include/core/core.h
  include/core/callable.h
  include/core/str_helper.h
  include/core/expected_helper.h
  include/core/variant_helper.h
  include/core/vec_helper.h
  include/core/log.h
  log.cpp)

target_include_directories(core PUBLIC include)
target_link_libraries(core PUBLIC expected random fmt::fmt)
