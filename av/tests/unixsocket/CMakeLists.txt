
include(GoogleTest)

SophosAddTest(TestSocketUtils
        TestSocketUtils.cpp
        TestStringUtils.cpp
        TestThreatReporterSocket.cpp
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
target_link_libraries(threatReporterClientExecutable  PUBLIC
        unixsocket)
target_include_directories(threatReporterClientExecutable PUBLIC ${CMAKE_SOURCE_DIR}/modules)
target_include_directories(threatReporterClientExecutable SYSTEM PRIVATE ${BOOSTPATH})

add_executable(threatReporterServerExecutable
        ../unixsocket/ThreatReporterTestExecutable/threatReporterServer.cpp)
target_link_libraries(threatReporterServerExecutable  PUBLIC
        unixsocket)
target_include_directories(threatReporterServerExecutable PUBLIC ${CMAKE_SOURCE_DIR}/modules)
target_include_directories(threatReporterServerExecutable SYSTEM PRIVATE ${BOOSTPATH})