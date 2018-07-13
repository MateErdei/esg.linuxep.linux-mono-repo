include(GoogleTest)
add_library(filesystemimpltests SHARED
        MockFileSystem.h
        test.cpp)
target_include_directories(filesystemimpltests SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include")
target_include_directories(filesystemimpltests BEFORE PUBLIC "${gtest_SOURCE_DIR}")
target_link_libraries(filesystemimpltests PUBLIC filesystemimpl gtest gtest_main gmock)

add_executable(FileSystemImplTests TestFileSystemImpl.cpp)
target_include_directories(FileSystemImplTests SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include")
target_include_directories(FileSystemImplTests BEFORE PUBLIC "${gtest_SOURCE_DIR}")
target_link_libraries(FileSystemImplTests gtest gtest_main)
target_link_libraries(FileSystemImplTests filesystemimpl testhelpers)
gtest_discover_tests(FileSystemImplTests)