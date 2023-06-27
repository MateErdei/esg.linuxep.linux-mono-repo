# We need to tell gtest we have abseil libs because of how it's built in mono repo.
add_compile_definitions(GTEST_HAS_ABSL)

add_custom_target(echogTest
        COMMAND echo gtest_SOURCE_DIR=${gtest_SOURCE_DIR}
        COMMAND ls -l ${gtest_SOURCE_DIR}/include/gtest/gtest.h
        )
add_dependencies(echogTest copy_libs)
add_subdirectory(manualTools)
add_subdirectory(cmscrouter)
add_subdirectory(Common)
add_subdirectory(diagnose)
add_subdirectory(SulDownloader)
add_subdirectory(sdu)
add_subdirectory(Installer)
add_subdirectory(ManagementAgent)
add_subdirectory(ManagementAgentImpl)
add_subdirectory(mcsrouter)
add_subdirectory(Telemetry)
add_subdirectory(TelemetryScheduler)
add_subdirectory(ResponseActions)
add_subdirectory(UpdateScheduler)
add_subdirectory(watchdog)
add_subdirectory(wdctl)
add_subdirectory(CentralRegistration)


if (BUILD_FUZZ_TESTS)
    message("Building fuzz tests ${BUILD_FUZZ_TESTS}")
    add_subdirectory(FuzzTests)
endif()

add_subdirectory(LibFuzzerTests)

add_custom_target(kcov
        COMMAND bash ${PROJECT_SOURCE_DIR}/build/kcov/run-kcov.sh ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/tests ${PROJECT_BINARY_DIR}/kcov_output ${PROJECT_BINARY_DIR}/tests
        )

add_custom_target(kcov-all
        COMMAND bash ${PROJECT_SOURCE_DIR}/build/kcov/run-kcov-all.sh ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/tests ${PROJECT_BINARY_DIR}/kcov_output ${PROJECT_BINARY_DIR}/tests
        )
