add_library(HttpRequestsImpl SHARED
        ../HttpRequests/IHttpRequester.h
        HttpRequesterImpl.h
        HttpRequesterImpl.cpp
        CurlFunctionsProvider.h
        CurlFunctionsProvider.cpp
        Logger.h
        Logger.cpp)

include_directories(HttpRequestsImpl BEFORE ${CURL_INCLUDE_DIR} ${LOG4CPLUS_INCLUDE_DIR} ${OPENSSL_INCLUDE_DIR} ../HttpRequests)

target_link_libraries(HttpRequestsImpl PUBLIC
        -L"${CMAKE_BINARY_DIR}/libs"
        ssl
        crypto
        ${CURL_LIBRARY}
        ${LOG4CPLUS_LIBRARY}
        logging
        )

SET_TARGET_PROPERTIES(HttpRequestsImpl
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

install(TARGETS HttpRequestsImpl
        LIBRARY DESTINATION files/base/lib64)

# Suldownloader that is called from the thin installer uses this lib
install(TARGETS HttpRequestsImpl LIBRARY DESTINATION ${THININSTALLER_DEPS_DIR_LIB64})

add_dependencies(HttpRequestsImpl copy_libs)
