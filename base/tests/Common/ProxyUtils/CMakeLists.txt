include(GoogleTest)
add_executable(TestProxyUtils
       TestProxyUtils.cpp)
target_include_directories(TestProxyUtils SYSTEM BEFORE PUBLIC ${GTEST_INCLUDE} ${GMOCK_INCLUDE})
target_link_libraries(TestProxyUtils
        ${GTEST_LIBRARY}
        ${GTEST_MAIN_LIBRARY}
        ${GMOCK_LIBRARY}
        utilityimpl
        ProxyUtilsLib
        filesystemimpl
        obfuscationimpl
        testhelpers)
gtest_discover_tests(TestProxyUtils)
SET_TARGET_PROPERTIES(TestProxyUtils
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )
