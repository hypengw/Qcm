find_package(CURL REQUIRED)

add_library(request
include/request/request.h
include/request/response.h
include/request/response_p.h
include/request/session.h
include/request/session_p.h
include/request/type.h

request_p.h request.cpp
response.cpp
session.cpp
curl_easy.h curl_easy.cpp
curl_multi.h curl_multi.cpp
curl_error.h
type.cpp
)
target_include_directories(request PUBLIC include PRIVATE include/request)
target_link_libraries(request PRIVATE ${CURL_LIBRARIES} asio_helper core fmt::fmt)
set_property(TARGET request PROPERTY POSITION_INDEPENDENT_CODE ON)
