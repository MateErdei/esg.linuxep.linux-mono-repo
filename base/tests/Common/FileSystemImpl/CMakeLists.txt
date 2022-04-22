include(GoogleTest)

add_executable(filesystemimpltestsexe
        TestFileSystemImpl.cpp
        TestFilePermissionsImpl.cpp
        TestTempDir.cpp
        TestPidLockFile.cpp
        MockPidLockFileUtils.h
        )
target_include_directories(filesystemimpltestsexe SYSTEM BEFORE PUBLIC  ${GMOCK_INCLUDE} ${GTEST_INCLUDE} ${CMAKE_SOURCE_DIR})
target_link_libraries(filesystemimpltestsexe ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY})
target_link_libraries(filesystemimpltestsexe filesystemimpl testhelpers)
gtest_discover_tests(filesystemimpltestsexe)
SET_TARGET_PROPERTIES(filesystemimpltestsexe
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )