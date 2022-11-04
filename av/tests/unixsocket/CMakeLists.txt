if (NOT DEFINED ENV{USE_LIBFUZZER})
    SophosAddTest(TestSocketUtils
            ../common/LogInitializedTests.cpp
            TestSocketUtils.cpp
            PROJECTS unixsocket
            )

    SophosAddTest(TestProcessControllerServerSocket
            ../common/LogInitializedTests.cpp
            TestProcessControllerServerConnectionThread.cpp
            TestProcessControllerServerSocket.cpp
            PROJECTS unixsocket
            )


    SophosAddTest(TestThreatDetectorSocket
            ../common/LogInitializedTests.cpp
            TestScanningServerConnectionThread.cpp
            TestThreatDetectorSocket.cpp
            PROJECTS unixsocket
            )

    SophosAddTest(TestThreatReporterSocket
            ../common/LogInitializedTests.cpp
            TestThreatReporterServerConnectionThread.cpp
            TestThreatReporterSocket.cpp
            PROJECTS unixsocket
            )

    SophosAddTest(TestUnixSocket
            ../common/LogInitializedTests.cpp
            TestUpdateCompleteClientSocketThread.cpp
            PROJECTS unixsocket
            )

    SophosAddTest(TestSafeStoreSocket
            ../common/LogInitializedTests.cpp
            TestSafeStoreSocket.cpp
            PROJECTS unixsocket
            )

    add_executable(ProcessControlClientExecutable
            ../unixsocket/ProcessControlTestExecutable/ProcessControllClient.cpp)
    target_link_libraries(ProcessControlClientExecutable  PUBLIC
            unixsocket)
    target_include_directories(ProcessControlClientExecutable PUBLIC ${CMAKE_SOURCE_DIR}/modules)
    target_include_directories(ProcessControlClientExecutable SYSTEM PRIVATE ${BOOSTPATH})

    add_executable(threatReporterClientExecutable
            ../unixsocket/ThreatReporterTestExecutable/threatReporterClient.cpp)
    target_link_libraries(threatReporterClientExecutable  PUBLIC
            unixsocket)
    target_include_directories(threatReporterClientExecutable PUBLIC ${CMAKE_SOURCE_DIR}/modules)
    target_include_directories(threatReporterClientExecutable SYSTEM PRIVATE ${BOOSTPATH})


    add_executable(UpdateCompleteClient
            ../unixsocket/UpdateCompleteTestExecutable/UpdateCompleteClient.cpp)
    target_link_libraries(UpdateCompleteClient  PUBLIC
            unixsocket)

endif (NOT DEFINED ENV{USE_LIBFUZZER})

add_executable(ProcessControlServerExecutable
        ../unixsocket/ProcessControlTestExecutable/ProcessControlServer.cpp)
target_link_libraries(ProcessControlServerExecutable  PUBLIC
        unixsocket)
target_include_directories(ProcessControlServerExecutable PUBLIC ${CMAKE_SOURCE_DIR}/modules)
target_include_directories(ProcessControlServerExecutable SYSTEM PRIVATE ${BOOSTPATH})

add_executable(threatReporterServerExecutable
        ../unixsocket/ThreatReporterTestExecutable/threatReporterServer.cpp)
target_link_libraries(threatReporterServerExecutable  PUBLIC
        unixsocket)
target_include_directories(threatReporterServerExecutable PUBLIC ${CMAKE_SOURCE_DIR}/modules)
target_include_directories(threatReporterServerExecutable SYSTEM PRIVATE ${BOOSTPATH})

add_executable(threatDetectorServerExecutable
        ../unixsocket/ThreatDetectorTestExecutable/threatDetectorServer.cpp)
target_link_libraries(threatDetectorServerExecutable  PUBLIC
        unixsocket ${STD_FILESYSTEM_IF_REQUIRED})
target_include_directories(threatDetectorServerExecutable PUBLIC ${CMAKE_SOURCE_DIR}/modules)
target_include_directories(threatDetectorServerExecutable SYSTEM PRIVATE ${BOOSTPATH})

add_executable(threatDetectorClientExecutable
        ThreatDetectorTestExecutable/threatDetectorClient.cpp
        ThreatDetectorTestExecutable/FakeServerConnectionThread.cpp
        ThreatDetectorTestExecutable/FakeServerConnectionThread.h
        ThreatDetectorTestExecutable/FakeServerSocket.cpp
        ThreatDetectorTestExecutable/FakeServerSocket.h)
target_link_libraries(threatDetectorClientExecutable  PUBLIC
        unixsocket avscannerimpl ${STD_FILESYSTEM_IF_REQUIRED})
target_include_directories(threatDetectorClientExecutable PUBLIC ${CMAKE_SOURCE_DIR}/modules)
target_include_directories(threatDetectorClientExecutable SYSTEM PRIVATE ${BOOSTPATH})

if (DEFINED ENV{USE_LIBFUZZER})
SET_TARGET_PROPERTIES( threatDetectorClientExecutable
        threatDetectorServerExecutable
        threatReporterServerExecutable
        ProcessControlServerExecutable
        PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../../../base-sdds/files/base/lib64/"
        )
install(TARGETS threatDetectorClientExecutable
        threatDetectorServerExecutable
        threatReporterServerExecutable
        ProcessControlServerExecutable
        DESTINATION files/plugins/${PLUGIN_NAME}/bin
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
        )

endif (DEFINED ENV{USE_LIBFUZZER})
