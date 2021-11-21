if( NOT EXISTS ${CMAKE_SOURCE_DIR}/tests/googletest )
    message( FATAL_ERROR "googletest directory not present. \nPlease, run ${CMAKE_SOURCE_DIR}/build.sh which will setup the directory")
endif()
add_subdirectory(googletest EXCLUDE_FROM_ALL)
add_custom_target(echogTest
        COMMAND echo gtest_SOURCE_DIR=${gtest_SOURCE_DIR}
        COMMAND ls -l ${gtest_SOURCE_DIR}/include/gtest/gtest.h
        )
add_subdirectory(plugin)
add_subdirectory(subscriber)
add_subdirectory(eventjournal)
add_subdirectory(EventQueueLib)
add_subdirectory(EventWriterWorker)
add_subdirectory(Heartbeat)
add_subdirectory(Helpers)
if (BUILD_FUZZ_TESTS)
add_subdirectory(FuzzerTests)
endif()

add_custom_target(kcov
        COMMAND bash ${PROJECT_SOURCE_DIR}/../everest-base/tools/run-kcov.sh ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/tests ${PROJECT_BINARY_DIR}/kcov_output ${PROJECT_BINARY_DIR}/tests
        )

add_custom_target(kcov-all
        COMMAND bash ${PROJECT_SOURCE_DIR}/../everest-base/tools/run-kcov-all.sh ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/tests ${PROJECT_BINARY_DIR}/kcov_output ${PROJECT_BINARY_DIR}/tests
        )