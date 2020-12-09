add_library(sslimpl SHARED
        Md5Calc.cpp Md5Calc.h)
message("opensslinclude: ${OPENSSL_INCLUDE_DIR}")
target_include_directories(sslimpl PUBLIC ${OPENSSL_INCLUDE_DIR})
target_link_libraries(sslimpl PUBLIC ${OPENSSLCRYPTO_LIBRARY})

SET_TARGET_PROPERTIES(sslimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS sslimpl
        LIBRARY DESTINATION files/base/lib64)