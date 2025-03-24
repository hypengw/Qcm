cmake_minimum_required(VERSION 3.28)

find_package(Qt6 REQUIRED COMPONENTS Core Qml)

add_library(meta_model STATIC src/qmetaobjectmodel.cpp src/moc.cpp)

target_compile_features(meta_model PUBLIC cxx_std_20)
set_target_properties(meta_model PROPERTIES AUTOMOC ON)
target_include_directories(meta_model PUBLIC include)
target_link_libraries(meta_model PUBLIC Qt6::Core Qt6::Qml core)
