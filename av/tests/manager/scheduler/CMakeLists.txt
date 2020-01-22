include(GoogleTest)

SophosAddTest(TestScheduledScanConfiguration
        TestScheduledScanConfiguration
        PROJECTS scanscheduler
        LIBS ${pluginapilib} ${testhelperslib} rt
        INC_DIRS ${CMAKE_CURRENT_BINARY_DIR} ${pluginapiinclude} ${testhelpersinclude}
            ${CMAKE_SOURCE_DIR}/modules
        )

add_dependencies(TestScheduledScanConfiguration copy_libs)

SET_TARGET_PROPERTIES(TestScheduledScanConfiguration
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )
