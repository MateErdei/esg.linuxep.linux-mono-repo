include(GoogleTest)
add_library(filesystemimpltests SHARED
        MockFileSystem.h)

target_include_directories(filesystemimpltests SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include")
target_include_directories(filesystemimpltests BEFORE PUBLIC "${gtest_SOURCE_DIR}")
target_link_libraries(filesystemimpltests PUBLIC filesystemimpl gtest gtest_main gmock)

add_executable(filesystemimpltestsexe TestFileSystemImpl.cpp)
target_include_directories(filesystemimpltestsexe SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include")
target_include_directories(filesystemimpltestsexe BEFORE PUBLIC "${gtest_SOURCE_DIR}")
target_link_libraries(filesystemimpltestsexe gtest gtest_main)
target_link_libraries(filesystemimpltestsexe filesystemimpl testhelpers)
gtest_discover_tests(filesystemimpltestsexe)