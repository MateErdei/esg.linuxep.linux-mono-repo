include(GoogleTest)

add_library(testhelpersobject OBJECT
        FakeTimeUtils.cpp
        FakeTimeUtils.h
        FilePermissionsReplaceAndRestore.cpp
        FilePermissionsReplaceAndRestore.h
        FileSystemReplaceAndRestore.cpp
        FileSystemReplaceAndRestore.h
        LogInitializedTests.h
        MemoryAppender.h
        MockApiBaseServices.h
        MockCurlWrapper.h
        MockFilePermissions.h
        MockFileSystem.h
        MockHttpRequester.h
        MockPlatformUtils.h
        MockPoller.h
        MockPollerFactory.h
        MockSignalHandler.h
        MockSysCalls.h
        MockSysCallsFactory.h
        MockSystemUtils.h
        MockZipUtils.h
        MockZmqContext.h
        MockiNotifyWrapper.h
        OutputToLog.cpp
        OutputToLog.h
        TempDir.cpp
        TempDir.h
        TestExecutionSynchronizer.cpp
        TestExecutionSynchronizer.h
        TestExecutionSynchronizer.h
        TestMacros.cpp
        TestMacros.h
        TestSpecificDirectory.h
        )

target_include_directories(testhelpersobject SYSTEM BEFORE PUBLIC ${GMOCK_INCLUDE} ${GTEST_INCLUDE})
target_link_libraries(testhelpersobject INTERFACE ${GTEST_LIBRARY} ${LOG4CPLUS_LIBRARY} filesystemimpl logging)

add_library(testhelpers SHARED)

target_include_directories(testhelpers SYSTEM BEFORE PUBLIC ${GMOCK_INCLUDE} ${GTEST_INCLUDE} ${LOG4CPLUS_INCLUDE_DIR})

target_link_libraries(testhelpers
        PRIVATE testhelpersobject
        PUBLIC filesystemimpl applicationconfigurationimpl ${GTEST_LIBRARY}  pluginapiimpl)
SET_TARGET_PROPERTIES( testhelpers
        PROPERTIES INSTALL_RPATH "$ORIGIN")

INSTALL(TARGETS testhelpers LIBRARY DESTINATION ${CMAKE_BINARY_DIR}/SystemProductTestOutput)
