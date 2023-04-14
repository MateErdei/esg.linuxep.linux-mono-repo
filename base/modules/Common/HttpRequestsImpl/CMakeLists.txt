add_library(HttpRequestsImplobject OBJECT
        ../HttpRequests/IHttpRequester.h
        HttpRequesterImpl.h
        HttpRequesterImpl.cpp
        CurlFunctionsProvider.h
        CurlFunctionsProvider.cpp
        Logger.h
        Logger.cpp)

include_directories(HttpRequestsImplobject BEFORE ${LOG4CPLUS_INCLUDE_DIR})
target_link_libraries(HttpRequestsImplobject PUBLIC
        curlwrapper
        curl_library
        ${LOG4CPLUS_LIBRARY}
        logging
        )

add_library(HttpRequestsImpl SHARED)
target_link_libraries(HttpRequestsImpl PUBLIC HttpRequestsImplobject)

SET_TARGET_PROPERTIES(HttpRequestsImpl
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

install(TARGETS HttpRequestsImpl
        LIBRARY DESTINATION ${BASE_DIST_PREFIX}/files/base/lib64)

add_dependencies(HttpRequestsImpl copy_libs)
