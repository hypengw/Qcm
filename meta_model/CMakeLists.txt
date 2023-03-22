add_library(meta_model STATIC 
    ./include/meta_model/qgadgetlistmodel.h
    include/meta_model/qmetaobjectmodel.h qmetaobjectmodel.cpp
    )

target_include_directories(meta_model PUBLIC include)

target_link_libraries(
  meta_model
  PUBLIC Qt6::Core core)
set_property(TARGET meta_model PROPERTY POSITION_INDEPENDENT_CODE ON)
