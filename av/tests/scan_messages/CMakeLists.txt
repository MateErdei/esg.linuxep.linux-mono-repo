
include(GoogleTest)

SophosAddTest(TestScanMessages
        TestScanMessages.cpp
        TestThreatDetectedMessages.cpp
        TestScanResponseMessages.cpp
        PROJECTS scanmessages
        LIBS
        "${CAPNPROTO_LIBRARY_DIR}/libcapnpc.a"
        "${CAPNPROTO_LIBRARY_DIR}/libcapnp-rpc.a"
        "${CAPNPROTO_LIBRARY_DIR}/libcapnp.a"
        "${CAPNPROTO_LIBRARY_DIR}/libkj-async.a"
        "${CAPNPROTO_LIBRARY_DIR}/libkj.a"
        INC_DIRS
        "${CAPNPROTO_INCLUDE_DIR}"
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_BINARY_DIR}/modules/scan_messages
        ${CMAKE_SOURCE_DIR}/modules
        )

SET_TARGET_PROPERTIES(TestScanMessages
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )
