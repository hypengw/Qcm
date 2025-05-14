set(gtest_force_shared_crt ON)
find_package(GTest QUIET)
if(NOT GTest_FOUND)
  include(FetchContent)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1)
  FetchContent_MakeAvailable(googletest)
endif()

add_executable(qcm_test src/asio.cpp src/main.cpp src/log.cpp)

target_link_libraries(qcm_test PRIVATE core.asio GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(qcm_test)
