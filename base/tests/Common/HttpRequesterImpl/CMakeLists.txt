include(GoogleTest)

# HTTP requester lib unit tests
add_executable(HttpRequesterTests
        HttpRequesterTests.cpp
        CurlFunctionsProviderTests.cpp
        ${PROJECT_SOURCE_DIR}/modules/Common/CurlWrapper/CurlWrapper.h
        ${PROJECT_SOURCE_DIR}/modules/Common/CurlWrapper/CurlWrapper.cpp
        )

target_include_directories(HttpRequesterTests SYSTEM BEFORE PUBLIC
        ${PROTOBUF_INCLUDE_DIR}
        "${gtest_SOURCE_DIR}/include"
        "${gmock_SOURCE_DIR}/include"
        ${CURL_INCLUDE_DIR})

target_link_libraries(HttpRequesterTests
        HttpRequestsImpl
        ${OPENSSL_LIBRARY}
        ${OPENSSL_CRYPTO_LIBRARY}
        gtest
        gtest_main)

target_link_libraries(HttpRequesterTests gtest gmock gtest_main -L"${CMAKE_BINARY_DIR}/libs" ssl crypto testhelpers)
gtest_discover_tests(HttpRequesterTests)

#------------------------

# Live network tests
add_executable(HttpRequesterLiveNetworkTests
        HttpRequesterLiveNetworkTests.cpp
        ${PROJECT_SOURCE_DIR}/modules/Common/CurlWrapper/CurlWrapper.h
        ${PROJECT_SOURCE_DIR}/modules/Common/CurlWrapper/CurlWrapper.cpp
        )

target_include_directories(HttpRequesterLiveNetworkTests SYSTEM BEFORE PUBLIC
        ${PROTOBUF_INCLUDE_DIR}
        "${gtest_SOURCE_DIR}/include"
        "${gmock_SOURCE_DIR}/include"
        ${CURL_INCLUDE_DIR})

target_link_libraries(HttpRequesterLiveNetworkTests
        HttpRequestsImpl
        gtest
        gtest_main)

set_target_properties(HttpRequesterLiveNetworkTests PROPERTIES
        INSTALL_RPATH "/opt/sophos-spl/base/lib64/" )

install(TARGETS HttpRequesterLiveNetworkTests
        DESTINATION ${CMAKE_BINARY_DIR}/SystemProductTestOutput)

target_link_libraries(HttpRequesterLiveNetworkTests gtest gmock gtest_main -L"${CMAKE_BINARY_DIR}/libs" ssl crypto testhelpers)
