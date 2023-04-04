add_library(evpoll INTERFACE ./include/evpoll/platform/epoll.h
            ./include/evpoll/evpoll.h ./include/evpoll/common.h)

target_include_directories(evpoll INTERFACE include)
target_link_libraries(evpoll INTERFACE core)
