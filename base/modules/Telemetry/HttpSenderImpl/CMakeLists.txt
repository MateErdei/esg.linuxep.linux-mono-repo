add_library(httpsenderimpl SHARED
        ../HttpSender/ICurlWrapper.h
        CurlWrapper.h
        CurlWrapper.cpp
        ../HttpSender/IHttpSender.h
        HttpSender.h
        HttpSender.cpp
        )

include_directories(httpsenderimpl BEFORE ${OPENSSL_INCLUDE_DIR} ${LOG4CPLUS_INCLUDE_DIR} ${CURL_INCLUDE_DIR})

target_link_libraries(httpsenderimpl PUBLIC
        -L"${CMAKE_BINARY_DIR}/libs"
        ssl
        crypto
        ${CURL_LIBRARY}
        ${LOG4CPLUS_LIBRARY}
        telemetryloggerimpl
        )

SET_TARGET_PROPERTIES(httpsenderimpl
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

install(TARGETS httpsenderimpl
        LIBRARY DESTINATION files/base/lib64)

add_dependencies(httpsenderimpl copy_libs)