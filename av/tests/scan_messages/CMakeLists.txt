
SophosAddTest(TestScanMessages
        TestScanMessages.cpp
        TestThreatDetectedMessages.cpp
        TestScanResponseMessages.cpp
        PROJECTS scanmessages
        INC_DIRS
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_BINARY_DIR}/modules/scan_messages
        ${CMAKE_SOURCE_DIR}/modules
        )

