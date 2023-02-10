include(GoogleTest)
add_executable(TestStatusCacheImpl
        TestStatusCache.cpp
       )
target_include_directories(TestStatusCacheImpl SYSTEM BEFORE PUBLIC ${GTEST_INCLUDE} ${GMOCK_INCLUDE})
target_include_directories(TestStatusCacheImpl BEFORE PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(TestStatusCacheImpl ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY})
target_link_libraries(TestStatusCacheImpl statuscacheimpl testhelpers)
gtest_discover_tests(TestStatusCacheImpl)
SET_TARGET_PROPERTIES(TestStatusCacheImpl
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )