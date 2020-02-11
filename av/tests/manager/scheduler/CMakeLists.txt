include(GoogleTest)

SophosAddTest(TestScheduledScan
        SetupLogging.cpp
        TestScanScheduler.cpp
        TestScheduleCalculations.cpp
        TestScheduledScan.cpp
        TestScheduledScanConfiguration.cpp
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
