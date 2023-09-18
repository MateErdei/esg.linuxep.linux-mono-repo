if (NOT DEFINED ENV{USE_LIBFUZZER})
    SophosAddTest(TestSocketUtils
            ../common/LogInitializedTests.cpp
            TestSocketUtils.cpp
            TestReadBufferAsync.cpp
            TestReadLengthAsync.cpp
            TestReadFully.cpp
            PROJECTS unixsocket
            INC_DIRS ${testhelpersinclude}
            )

    SophosAddTest(TestProcessControllerServerSocket
            ../common/LogInitializedTests.cpp
            TestProcessControllerServerConnectionThread.cpp
            TestProcessControllerServerSocket.cpp
            PROJECTS unixsocket
            INC_DIRS ${testhelpersinclude}
            )


    SophosAddTest(TestThreatDetectorSocket
            ../common/LogInitializedTests.cpp
            TestScanningServerConnectionThread.cpp
            TestThreatDetectorSocket.cpp
            TestThreatDetectedMessageUtils.cpp
            threatDetectorSocket/FakeScanningServer.cpp
            threatDetectorSocket/FakeScanningServer.h
            threatDetectorSocket/TestScanningClientSocket.cpp
            PROJECTS unixsocket
            INC_DIRS ${testhelpersinclude}
            LIBS  ${testhelperslib}
            )

    SophosAddTest(TestThreatReporterSocket
            ../common/LogInitializedTests.cpp
            TestThreatReporterServerConnectionThread.cpp
            TestThreatReporterSocket.cpp
            PROJECTS unixsocket
            INC_DIRS ${testhelpersinclude}
            )

    SophosAddTest(TestUnixSocket
            ../common/LogInitializedTests.cpp
            TestUpdateCompleteClientSocketThread.cpp
            PROJECTS unixsocket
            )

    SophosAddTest(TestSafeStoreSocket
            ../common/LogInitializedTests.cpp
            TestSafeStoreServerConnectionThread.cpp
            TestSafeStoreSocket.cpp
            PROJECTS unixsocket safestoreimpl
            INC_DIRS ${testhelpersinclude}
            LIBS ${log4cpluslib} ${testhelperslib}
            )

    SophosAddTest(TestSafeStoreRescanSocket
            ../common/LogInitializedTests.cpp
            TestSafeStoreRescanSocket.cpp
            PROJECTS unixsocket
            )

    SophosAddTest(TestRestoreReportingSocket
            ../common/LogInitializedTests.cpp
            TestRestoreReportingSocket.cpp
            PROJECTS unixsocket
            )

    SophosAddTest(TestMetadataRescanSocket
            ../common/LogInitializedTests.cpp
            TestMetadataRescanSocket.cpp
            PROJECTS unixsocket
            INC_DIRS ${testhelpersinclude}
            )

    add_executable(ProcessControlClientExecutable
            ProcessControlTestExecutable/ProcessControlClient.cpp)
    target_link_libraries(ProcessControlClientExecutable  PUBLIC
            unixsocket)
    target_include_directories(ProcessControlClientExecutable PUBLIC ${CMAKE_SOURCE_DIR}/modules)

    add_executable(threatReporterClientExecutable
            ../unixsocket/ThreatReporterTestExecutable/threatReporterClient.cpp)
    target_link_libraries(threatReporterClientExecutable  PUBLIC
            unixsocket)
    target_include_directories(threatReporterClientExecutable PUBLIC ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/modules)


    add_executable(UpdateCompleteClient
            ../unixsocket/UpdateCompleteTestExecutable/UpdateCompleteClient.cpp)
    target_link_libraries(UpdateCompleteClient  PUBLIC
            unixsocket)

    add_subdirectory(SendThreatDetectedExecutable)
endif (NOT DEFINED ENV{USE_LIBFUZZER})

add_executable(ProcessControlServerExecutable
        ../unixsocket/ProcessControlTestExecutable/ProcessControlServer.cpp)
target_link_libraries(ProcessControlServerExecutable  PUBLIC
        unixsocket)
target_include_directories(ProcessControlServerExecutable PUBLIC ${CMAKE_SOURCE_DIR}/modules)

add_executable(threatReporterServerExecutable
        ../unixsocket/ThreatReporterTestExecutable/threatReporterServer.cpp)
target_link_libraries(threatReporterServerExecutable  PUBLIC
        unixsocket)
target_include_directories(threatReporterServerExecutable PUBLIC ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/modules)

add_executable(threatDetectorServerExecutable
        ThreatDetectorTestExecutable/threatDetectorServer.cpp
        ../common/FakeThreatScannerFactory.h
)
target_link_libraries(threatDetectorServerExecutable  PUBLIC
        unixsocket ${STD_FILESYSTEM_IF_REQUIRED})
target_include_directories(threatDetectorServerExecutable PUBLIC ${CMAKE_SOURCE_DIR}/modules ${CMAKE_SOURCE_DIR})

add_executable(threatDetectorClientExecutable
        ThreatDetectorTestExecutable/threatDetectorClient.cpp
        ThreatDetectorTestExecutable/FakeServerConnectionThread.cpp
        ThreatDetectorTestExecutable/FakeServerConnectionThread.h
        ThreatDetectorTestExecutable/FakeServerSocket.cpp
        ThreatDetectorTestExecutable/FakeServerSocket.h)
target_link_libraries(threatDetectorClientExecutable  PUBLIC
        unixsocket avscannerimpl ${STD_FILESYSTEM_IF_REQUIRED})
target_include_directories(threatDetectorClientExecutable PUBLIC ${CMAKE_SOURCE_DIR}/modules)

if (DEFINED ENV{USE_LIBFUZZER})
# These RPATHs are only used for fuzzing!
set(FUZZ_RPATH "$ORIGIN:$ORIGIN/../lib64:${CMAKE_BINARY_DIR}/libs:/build/input/gcc/lib64")
SET_TARGET_PROPERTIES( threatDetectorClientExecutable
        threatDetectorServerExecutable
        threatReporterServerExecutable
        ProcessControlServerExecutable
        PROPERTIES
        BUILD_RPATH "${FUZZ_RPATH}"
        INSTALL_RPATH "${FUZZ_RPATH}"
        )
install(TARGETS threatDetectorClientExecutable
        threatDetectorServerExecutable
        threatReporterServerExecutable
        ProcessControlServerExecutable
        DESTINATION files/plugins/${PLUGIN_NAME}/bin
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
        )

endif (DEFINED ENV{USE_LIBFUZZER})
