add_library(curlwrapperobject OBJECT
        CurlWrapper.h
        CurlWrapper.cpp
        )

target_include_directories(curlwrapperobject PUBLIC ${CURL_INCLUDE_DIR})

add_library(curlwrapper SHARED $<TARGET_OBJECTS:curlwrapperobject>)

SET_TARGET_PROPERTIES(curlwrapper
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS curlwrapper
        LIBRARY DESTINATION ${BASE_DIST_PREFIX}/files/base/lib64)

install(TARGETS curlwrapper LIBRARY DESTINATION ${THININSTALLER_DEPS_DIR_LIB64})