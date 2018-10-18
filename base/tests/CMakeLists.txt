add_subdirectory(googletest EXCLUDE_FROM_ALL)
add_custom_target(echogTest
        COMMAND echo gtest_SOURCE_DIR=${gtest_SOURCE_DIR}
        COMMAND ls -l ${gtest_SOURCE_DIR}/include/gtest/gtest.h
        )
add_subdirectory(Common)
add_subdirectory(SulDownloader)
add_subdirectory(Installer)
add_subdirectory(ManagementAgent)
add_subdirectory(ManagementAgentImpl)
add_subdirectory(UpdateScheduler)
add_subdirectory(watchdog)
add_subdirectory(wdctl)

add_custom_target(kcov
        COMMAND bash ${PROJECT_SOURCE_DIR}/tools/run-kcov.sh ${PROJECT_BINARY_DIR} ${PROJECT_BINARY_DIR}/tests ${PROJECT_BINARY_DIR}/kcov_output ${PROJECT_BINARY_DIR}/tests
        )
