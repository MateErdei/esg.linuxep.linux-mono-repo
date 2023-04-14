add_library(sslimplobject OBJECT
        Digest.cpp
        Digest.h
        )
target_link_libraries(sslimplobject PUBLIC openssl_crypto)
add_library(sslimpl SHARED)
target_link_libraries(sslimpl PUBLIC sslimplobject)

SET_TARGET_PROPERTIES(sslimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS sslimpl
        LIBRARY DESTINATION ${BASE_DIST_PREFIX}/files/base/lib64)
