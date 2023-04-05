set(FMT_INSTALL OFF)
add_subdirectory(fmt)
add_subdirectory(json)
add_subdirectory(qr_code)


add_library(random INTERFACE)
target_include_directories(random INTERFACE random/include)

add_library(expected INTERFACE)
target_include_directories(expected INTERFACE expected/include)
add_library(tl::expected ALIAS expected)

target_compile_definitions(nlohmann_json INTERFACE JSON_DIAGNOSTICS=1)

add_library(asio INTERFACE)
# ASIO_ENABLE_HANDLER_TRACKING ASIO_NO_TYPEID
target_compile_definitions(asio INTERFACE ASIO_NO_DEPRECATED ASIO_NO_TYPEID)
target_include_directories(asio INTERFACE asio/asio/include)

set(md_dir material-color/cpp)
add_library(
  material_color STATIC
  ${md_dir}/scheme/scheme.cc
  ${md_dir}/palettes/core.cc
  ${md_dir}/palettes/tones.cc
  ${md_dir}/utils/utils.cc
  ${md_dir}/cam/cam.cc
  ${md_dir}/cam/hct.cc
  ${md_dir}/cam/hct_solver.cc
  ${md_dir}/cam/viewing_conditions.cc
  ${md_dir}/blend/blend.cc)
target_include_directories(material_color PUBLIC material-color)

add_library(ctre INTERFACE)
target_include_directories(ctre INTERFACE ctre/include)
