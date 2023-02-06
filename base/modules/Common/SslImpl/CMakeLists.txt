add_library(sslimplobject OBJECT
        Digest.cpp
        Digest.h
        )
message("opensslinclude: ${OPENSSL_INCLUDE_DIR}")
target_include_directories(sslimplobject PUBLIC ${OPENSSL_INCLUDE_DIR})
add_library(sslimpl SHARED $<TARGET_OBJECTS:sslimplobject>)

target_link_libraries(sslimpl PUBLIC ${OPENSSLCRYPTO_LIBRARY})

SET_TARGET_PROPERTIES(sslimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS sslimpl
        LIBRARY DESTINATION ${BASE_DIST_PREFIX}/files/base/lib64)

install(TARGETS sslimpl LIBRARY DESTINATION ${THININSTALLER_DEPS_DIR_LIB64})