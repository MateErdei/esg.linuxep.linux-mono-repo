
set(ENV{LD_LIBRARY_PATH} ${CMAKE_BINARY_DIR}/libs)

include(GoogleTest)
add_executable(HttpSenderTests
        HttpSenderTests.cpp
        MockCurlWrapper.h
        CurlWrapperTests.cpp)

target_include_directories(HttpSenderTests PUBLIC ${PROTOBUF_INCLUDE_DIR})
target_include_directories(HttpSenderTests SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include" ${CURL_INCLUDE_DIR})
target_include_directories(HttpSenderTests BEFORE PUBLIC "${gtest_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}")

target_link_libraries(HttpSenderTests gtest gmock gtest_main -L"${CMAKE_BINARY_DIR}/libs" ssl crypto httpsenderimpl testhelpers)
gtest_discover_tests(HttpSenderTests)

SET_TARGET_PROPERTIES(HttpSenderTests
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall")
