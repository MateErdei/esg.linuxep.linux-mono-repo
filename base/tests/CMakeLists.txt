add_subdirectory(googletest EXCLUDE_FROM_ALL)
add_custom_target(echogTest
        COMMAND echo gtest_SOURCE_DIR=${gtest_SOURCE_DIR}
        COMMAND ls -l ${gtest_SOURCE_DIR}/include/gtest/gtest.h
        )
add_subdirectory(Common)
add_subdirectory(SulDownloader)
add_subdirectory(ManagementAgent)