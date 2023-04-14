add_library(curlwrapperobject OBJECT
        CurlWrapper.h
        CurlWrapper.cpp
        )

target_link_libraries(curlwrapperobject PUBLIC curl_library)

add_library(curlwrapper SHARED)
target_link_libraries(curlwrapper PUBLIC curlwrapperobject)

SET_TARGET_PROPERTIES(curlwrapper
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS curlwrapper
        LIBRARY DESTINATION ${BASE_DIST_PREFIX}/files/base/lib64)

install(TARGETS curlwrapper LIBRARY DESTINATION ${THININSTALLER_DEPS_DIR_LIB64})