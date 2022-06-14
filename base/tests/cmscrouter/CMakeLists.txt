
set(ENV{LD_LIBRARY_PATH} ${CMAKE_BINARY_DIR}/libs)

include(GoogleTest)

add_executable(cMCSTests
        TestMcsHttpClient.cpp
        TestAgentAdapter.cpp
        MockAgentAdapter.h
        TestConfigOptions.cpp
        ${PROJECT_SOURCE_DIR}/modules/Common/CurlWrapper/CurlWrapper.h
        ${PROJECT_SOURCE_DIR}/modules/Common/CurlWrapper/CurlWrapper.cpp
)

target_include_directories(cMCSTests SYSTEM BEFORE PUBLIC
        ${CMAKE_SOURCE_DIR}
        "${gtest_SOURCE_DIR}/include"
        "${gmock_SOURCE_DIR}/include"
        ${CURL_INCLUDE_DIR})
target_link_libraries(cMCSTests gtest gmock gtest_main -L"${CMAKE_BINARY_DIR}/libs" testhelpers obfuscationimpl mcsimpl HttpRequestsImpl )

gtest_discover_tests(cMCSTests)

SET_TARGET_PROPERTIES(cMCSTests
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")