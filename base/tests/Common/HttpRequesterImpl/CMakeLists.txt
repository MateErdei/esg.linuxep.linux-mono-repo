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
        ${GMOCK_INCLUDE}
        ${GTEST_INCLUDE}
        )

target_link_libraries(HttpRequesterTests
        HttpRequestsImpl
        ${GTEST_LIBRARY}
        ${GTEST_MAIN_LIBRARY}
        testhelpers
        )

set_target_properties(HttpRequesterTests PROPERTIES
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs")
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
        ${GMOCK_INCLUDE}
        ${GTEST_INCLUDE}
        )

target_link_libraries(HttpRequesterLiveNetworkTests
        HttpRequestsImpl
        ${GTEST_LIBRARY}
        ${GTEST_MAIN_LIBRARY}
        testhelpers
        )

set_target_properties(HttpRequesterLiveNetworkTests PROPERTIES
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        INSTALL_RPATH "/opt/sophos-spl/base/lib64/:$ORIGIN" )

install(TARGETS HttpRequesterLiveNetworkTests
        DESTINATION ${CMAKE_BINARY_DIR}/SystemProductTestOutput)

install(FILES ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY}
        DESTINATION ${CMAKE_BINARY_DIR}/SystemProductTestOutput)