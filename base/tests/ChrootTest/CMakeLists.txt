add_executable(HttpDemo
        HttpsRequest.cpp)

target_include_directories(HttpDemo PUBLIC ${CURL_INCLUDE_DIR})
target_link_libraries(HttpDemo PUBLIC ${CURL_LIBRARY} httpsenderimpl securityutils)

include_directories(HttpDemo BEFORE ${CURL_INCLUDE_DIR} ${BOOST_INCLUDE_DIR} ${LOG4CPLUS_INCLUDE_DIR} "${PROJECT_SOURCE_DIR}")

target_link_libraries(HttpDemo PUBLIC
        securityutils
        httpsenderimpl
        telemetryconfigimpl
        testhelpers
        )
