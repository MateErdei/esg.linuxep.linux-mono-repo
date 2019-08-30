add_library(httpsenderimpl SHARED
        ../HttpSender/ICurlWrapper.h
        CurlWrapper.h
        CurlWrapper.cpp
        ../HttpSender/IHttpSender.h
        HttpSender.h
        HttpSender.cpp
        RequestConfig.h
        RequestConfig.cpp
        Logger.h
        Logger.cpp)

include_directories(httpsenderimpl BEFORE ${CURL_INCLUDE_DIR} ${LOG4CPLUS_INCLUDE_DIR} ${OPENSSL_INCLUDE_DIR})

target_link_libraries(httpsenderimpl PUBLIC
        -L"${CMAKE_BINARY_DIR}/libs"
        ssl
        crypto
        ${CURL_LIBRARY}
        ${LOG4CPLUS_LIBRARY}
        logging
        )

SET_TARGET_PROPERTIES(httpsenderimpl
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

install(TARGETS httpsenderimpl
        LIBRARY DESTINATION files/base/lib64)

add_dependencies(httpsenderimpl copy_libs)
