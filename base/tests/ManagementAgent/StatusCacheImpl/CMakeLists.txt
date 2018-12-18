include(GoogleTest)
add_executable(TestStatusCacheImpl
        TestStatusCache.cpp
        ../../Common/Logging/TestConsoleLoggingSetup.h
        ../../Common/Logging/TestConsoleLoggingSetup.cpp
       )
target_include_directories(TestStatusCacheImpl SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include")
target_include_directories(TestStatusCacheImpl BEFORE PUBLIC "${gtest_SOURCE_DIR}" "${gmock_SOURCE_DIR}" "${gmock_SOURCE_DIR}/include" ${CMAKE_SOURCE_DIR})
target_link_libraries(TestStatusCacheImpl gtest gtest_main gmock)
target_link_libraries(TestStatusCacheImpl statuscacheimpl testhelpers)
gtest_discover_tests(TestStatusCacheImpl)
