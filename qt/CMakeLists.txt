find_package(Qt6 REQUIRED COMPONENTS Quick ProtobufQuick ProtobufWellKnownTypes)

add_library(qt STATIC src/asio/context.cpp)

target_sources(
  qt
  PUBLIC FILE_SET
         all
         TYPE
         CXX_MODULES
         FILES
         src/mod.cppm
         src/qt.cppm
         src/kstore.cppm
         src/helper.cppm
         src/qobject_bindable_property_p.cppm
         src/asio/mod.cppm
         src/asio/forward.cppm
         src/asio/context.cppm
         src/asio/executor.cppm
         src/asio/watcher.cppm)

target_compile_features(qt PUBLIC cxx_std_20)
target_include_directories(qt PUBLIC include)
target_link_libraries(qt PUBLIC Qt6::Core Qt6::Quick core.asio kstore::kstore)
