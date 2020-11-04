include(GoogleTest)
add_executable(TestPersistentValue
        ${CMAKE_SOURCE_DIR}/modules/Common/PersistentValue/PersistentValue.h
        TestPersistentValue.cpp)
target_include_directories(TestPersistentValue SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include" ${CMAKE_SOURCE_DIR})
target_include_directories(TestPersistentValue BEFORE PUBLIC "${gtest_SOURCE_DIR}" "${gmock_SOURCE_DIR}" )
target_link_libraries(TestPersistentValue gtest gtest_main gmock threads filesystemimpl testhelpers)
gtest_discover_tests(TestPersistentValue)
