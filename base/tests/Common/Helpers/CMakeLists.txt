include(GoogleTest)

add_library(testhelpersobject OBJECT
        TempDir.cpp
        TempDir.h
        TestExecutionSynchronizer.cpp
        TestExecutionSynchronizer.h
        PubSubPathReplacement.cpp
        PubSubPathReplacement.h
        TestExecutionSynchronizer.h
        FilePermissionsReplaceAndRestore.h
        FilePermissionsReplaceAndRestore.cpp
        FileSystemReplaceAndRestore.h
        FileSystemReplaceAndRestore.cpp
        MockApiBaseServices.h
        MockFilePermissions.h
        MockFileSystem.h
        FakeTimeUtils.cpp
        FakeTimeUtils.h
        MockiNotifyWrapper.h
        )

target_include_directories(testhelpersobject SYSTEM BEFORE PUBLIC "${gmock_SOURCE_DIR}/include" "${gtest_SOURCE_DIR}/include")

add_library(testhelpers SHARED $<TARGET_OBJECTS:testhelpersobject>)

target_include_directories(testhelpers SYSTEM BEFORE PUBLIC "${gmock_SOURCE_DIR}/include" "${gtest_SOURCE_DIR}/include" "${gtest_SOURCE_DIR}")

target_link_libraries(testhelpers PUBLIC filesystemimpl applicationconfigurationimpl gmock gtest pluginapiimpl)

SET_TARGET_PROPERTIES( testhelpers
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS testhelpers
        LIBRARY DESTINATION files/base/lib64)
