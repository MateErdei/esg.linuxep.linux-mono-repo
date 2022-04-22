include(GoogleTest)
add_executable(TestPersistentValue
        ${CMAKE_SOURCE_DIR}/modules/Common/PersistentValue/PersistentValue.h
        TestPersistentValue.cpp)
target_include_directories(TestPersistentValue SYSTEM BEFORE PUBLIC ${GMOCK_INCLUDE} ${GTEST_INCLUDE} ${CMAKE_SOURCE_DIR})
target_link_libraries(TestPersistentValue ${GMOCK_LIBRARY} ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY} threads filesystemimpl testhelpers)
gtest_discover_tests(TestPersistentValue)
SET_TARGET_PROPERTIES(TestPersistentValue
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )