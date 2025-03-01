set(CMAKE_AUTOMOC ON)
add_library(
  meta_model STATIC
  include/meta_model/qgadgetlistmodel.h 
  include/meta_model/qobjectlistmodel.h 
  include/meta_model/qmetaobjectmodel.h
  qmetaobjectmodel.cpp
  moc.cpp)

target_include_directories(meta_model PUBLIC include)

target_link_libraries(meta_model PUBLIC Qt6::Core Qt6::Qml core)
