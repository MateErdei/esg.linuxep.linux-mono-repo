add_executable(HttpDemo
        HttpsRequest.cpp)

target_include_directories(HttpDemo PUBLIC ${CURL_INCLUDE_DIR})
target_link_libraries(HttpDemo PUBLIC ${CURL_LIBRARY} httpsenderimpl securityutils)

include_directories(HttpDemo BEFORE ${CURL_INCLUDE_DIR} ${BOOST_INCLUDE_DIR})

target_link_libraries(HttpDemo PUBLIC
        httpsenderimpl
        telemetryconfigimpl
        )
