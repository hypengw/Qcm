find_package(CURL REQUIRED)

add_library(
  request STATIC
  include/request/request.h
  include/request/response.h
  include/request/response_p.h
  include/request/session.h
  include/request/session_p.h
  include/request/type.h
  peg/uri.h
  peg/http.h
  connection.h
  request_p.h
  request.cpp
  response.cpp
  session.cpp
  session_share.cpp
  curl_easy.h
  curl_multi.h
  curl_error.h
  type.cpp
  peg.cpp)
target_include_directories(
  request
  PUBLIC include
  PRIVATE include/request .)
target_link_libraries(request PRIVATE CURL::libcurl asio_helper core fmt::fmt
                                      taocpp::pegtl)
