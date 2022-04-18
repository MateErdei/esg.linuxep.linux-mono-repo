
set(ENV{LD_LIBRARY_PATH} ${CMAKE_BINARY_DIR}/libs)

include(GoogleTest)
add_executable(HttpSenderTests
        HttpSenderTests.cpp
        ../Helpers/MockCurlWrapper.h)

target_include_directories(HttpSenderTests PUBLIC  ${CMAKE_SOURCE_DIR}/modules/Common/HttpSender)
target_include_directories(HttpSenderTests SYSTEM BEFORE PUBLIC ${PROTOBUF_INCLUDE_DIR} ${GTEST_INCLUDE} ${GMOCK_INCLUDE} ${CURL_INCLUDE_DIR} "${PROJECT_SOURCE_DIR}")

target_link_libraries(HttpSenderTests  ${GMOCK_LIBRARY} ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY} -L"${CMAKE_BINARY_DIR}/libs" ssl crypto httpsenderimpl testhelpers)
gtest_discover_tests(HttpSenderTests)

SET_TARGET_PROPERTIES(HttpSenderTests
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall")
