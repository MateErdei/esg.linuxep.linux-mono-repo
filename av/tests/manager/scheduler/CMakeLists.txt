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
        PROJECTS scanscheduler
        LIBS ${pluginapilib} ${testhelperslib} rt
        INC_DIRS ${CMAKE_CURRENT_BINARY_DIR} ${pluginapiinclude} ${testhelpersinclude}
            ${CMAKE_SOURCE_DIR}/modules
        )

add_dependencies(TestScheduledScan copy_libs)

SET_TARGET_PROPERTIES(TestScheduledScan
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )
