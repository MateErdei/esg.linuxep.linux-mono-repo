include(GoogleTest)

SophosAddTest(TestScheduledScan
        ../../common/LogInitializedTests.cpp
        TestDaySet.cpp
        TestScanRunner.cpp
        TestScanScheduler.cpp
        TestScanSerialiser.cpp
        TestScheduleCalculations.cpp
        TestScheduledScan.cpp
        TestScheduledScanConfiguration.cpp
        TestTimeSet.cpp
        PROJECTS scanscheduler common
        LIBS
        ${pluginapilib}
        ${testhelperslib}
        rt
        "${CAPNPROTO_LIBRARY_DIR}/libcapnpc.a"
        "${CAPNPROTO_LIBRARY_DIR}/libcapnp-rpc.a"
        "${CAPNPROTO_LIBRARY_DIR}/libcapnp.a"
        "${CAPNPROTO_LIBRARY_DIR}/libkj-async.a"
        "${CAPNPROTO_LIBRARY_DIR}/libkj.a"
        INC_DIRS ${CMAKE_CURRENT_BINARY_DIR} ${pluginapiinclude} ${testhelpersinclude}
            ${CMAKE_SOURCE_DIR}/modules
        )

add_dependencies(TestScheduledScan copy_libs)

SET_TARGET_PROPERTIES(TestScheduledScan
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )
