include(GoogleTest)
add_library(filepermissionsimpltests SHARED
        MockFilePermissions.h)

target_include_directories(filepermissionsimpltests SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include")
target_include_directories(filepermissionsimpltests BEFORE PUBLIC "${gtest_SOURCE_DIR}")
target_link_libraries(filepermissionsimpltests PUBLIC filepermissionsimpl gtest gtest_main gmock)

add_executable(filepermissionsimpltestsexe TestFilePermissionsImpl.cpp)
target_include_directories(filepermissionsimpltestsexe SYSTEM BEFORE PUBLIC "${gtest_SOURCE_DIR}/include" "${gmock_SOURCE_DIR}/include")
target_include_directories(filepermissionsimpltestsexe BEFORE PUBLIC "${gtest_SOURCE_DIR}")
target_link_libraries(filepermissionsimpltestsexe gtest gtest_main)
target_link_libraries(filepermissionsimpltestsexe filepermissionsimpl testhelpers)
gtest_discover_tests(filepermissionsimpltestsexe)