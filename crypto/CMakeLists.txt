find_package(OpenSSL REQUIRED)

add_library(crypto STATIC
    include/crypto/crypto.h
    crypto.cpp
)

target_include_directories(crypto PUBLIC include PRIVATE include/crypto)
target_link_libraries(crypto PUBLIC core PRIVATE OpenSSL::SSL OpenSSL::Crypto)
target_compile_definitions(crypto PRIVATE OPENSSL_API_COMPAT=30000)
set_property(TARGET crypto PROPERTY POSITION_INDEPENDENT_CODE ON)
