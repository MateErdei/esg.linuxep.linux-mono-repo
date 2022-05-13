add_custom_target(echogTest
        COMMAND echo gtest_SOURCE_DIR=${gtest_SOURCE_DIR}
        COMMAND ls -l ${gtest_SOURCE_DIR}/include/gtest/gtest.h
        )
include(GoogleTest)
add_subdirectory(plugin)
add_subdirectory(livequery)
add_subdirectory(manual)
add_subdirectory(osqueryclient)
add_subdirectory(osqueryextensions)
add_subdirectory(queryrunner)
add_subdirectory(eventjournalwrapperimpl)
if (BUILD_FUZZ_TESTS )
add_subdirectory(FuzzerTests)
endif()