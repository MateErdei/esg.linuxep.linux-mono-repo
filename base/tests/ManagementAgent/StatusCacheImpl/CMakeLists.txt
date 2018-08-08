include(GoogleTest)
add_executable(TestStatusCacheImpl
        ../StatusCacheImpl/TestStatusCache.cpp
       )
target_include_directories(TestStatusCacheImpl SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include")
target_include_directories(TestStatusCacheImpl BEFORE PUBLIC "${gtest_SOURCE_DIR}" "${gmock_SOURCE_DIR}" "${gmock_SOURCE_DIR}/include" ../../../)
target_link_libraries(TestStatusCacheImpl gtest gtest_main gmock)
target_link_libraries(TestStatusCacheImpl statuscacheimpl testhelpers applicationconfigurationimpl)
gtest_discover_tests(TestStatusCacheImpl)
