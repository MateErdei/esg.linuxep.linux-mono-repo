if (NOT EXISTS ${CMAKE_SOURCE_DIR}/tests/googletest)
    message( FATAL_ERROR "googletest directory not present. \nPlease, run ${CMAKE_SOURCE_DIR}/build.sh which will setup the directory")
endif()

add_subdirectory(googletest EXCLUDE_FROM_ALL)
add_custom_target(echogTest
        COMMAND echo gtest_SOURCE_DIR=${gtest_SOURCE_DIR}
        COMMAND ls -l ${gtest_SOURCE_DIR}/include/gtest/gtest.h
        )
add_subdirectory(manualTools)
add_subdirectory(Common)
add_subdirectory(diagnose)
add_subdirectory(SulDownloader)
add_subdirectory(Installer)
add_subdirectory(ManagementAgent)
add_subdirectory(ManagementAgentImpl)
add_subdirectory(mcsrouter)
add_subdirectory(Telemetry)
add_subdirectory(TelemetryScheduler)
add_subdirectory(UpdateScheduler)
add_subdirectory(watchdog)
add_subdirectory(wdctl)

if (BUILD_FUZZ_TESTS)
    message("Building fuzz tests ${BUILD_FUZZ_TESTS}")
    add_subdirectory(FuzzTests)
endif()

add_subdirectory(LibFuzzerTests)

add_custom_target(kcov
        COMMAND bash ${PROJECT_SOURCE_DIR}/tools/run-kcov.sh ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/tests ${PROJECT_BINARY_DIR}/kcov_output ${PROJECT_BINARY_DIR}/tests
        )

add_custom_target(kcov-all
        COMMAND bash ${PROJECT_SOURCE_DIR}/tools/run-kcov-all.sh ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/tests ${PROJECT_BINARY_DIR}/kcov_output ${PROJECT_BINARY_DIR}/tests
        )
