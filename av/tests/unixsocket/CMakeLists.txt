
include(GoogleTest)

SophosAddTest(TestSocketUtils
        ../common/LogInitializedTests.cpp
        TestSocketUtils.cpp
        TestStringUtils.cpp
        PROJECTS unixsocket avscannerimpl
        INC_DIRS
        ${CMAKE_SOURCE_DIR}/modules
        )
SET_TARGET_PROPERTIES(TestSocketUtils
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

add_executable(threatReporterClientExecutable
        ../unixsocket/ThreatReporterTestExecutable/threatReporterClient.cpp)
target_link_libraries(threatReporterClientExecutable  PUBLIC unixsocket)
target_include_directories(threatReporterClientExecutable PUBLIC ${CMAKE_SOURCE_DIR}/modules)

add_executable(threatReporterServerExecutable
        ../unixsocket/ThreatReporterTestExecutable/threatReporterServer.cpp)
target_link_libraries(threatReporterServerExecutable  PUBLIC unixsocket)
target_include_directories(threatReporterServerExecutable PUBLIC ${CMAKE_SOURCE_DIR}/modules)

add_executable(threatDetectorServerExecutable
        ../unixsocket/ThreatDetectorTestExecutable/threatDetectorServer.cpp)
target_link_libraries(threatDetectorServerExecutable  PUBLIC
        unixsocket ${STD_FILESYSTEM_IF_REQUIRED})
target_include_directories(threatDetectorServerExecutable PUBLIC ${CMAKE_SOURCE_DIR}/modules)
target_include_directories(threatDetectorServerExecutable SYSTEM PRIVATE ${BOOSTPATH})

SophosAddTest(TestThreatDetectorSocket
        ../common/LogInitializedTests.cpp
        TestScanningServerConnectionThread.cpp
        TestThreatDetectorSocket.cpp
        PROJECTS unixsocket
        INC_DIRS ${CMAKE_SOURCE_DIR}/modules ${CMAKE_SOURCE_DIR}
        )
SET_TARGET_PROPERTIES(TestThreatDetectorSocket
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

SophosAddTest(TestThreatReporterSocket
        ../common/LogInitializedTests.cpp
        TestThreatReporterServerConnectionThread.cpp
        TestThreatReporterSocket.cpp
        PROJECTS unixsocket
        INC_DIRS ${CMAKE_SOURCE_DIR}/modules ${CMAKE_SOURCE_DIR}
        )
SET_TARGET_PROPERTIES(TestThreatReporterSocket
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )
