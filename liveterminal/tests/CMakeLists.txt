# We need to tell gtest we have abseil libs because of how it's built in mono repo.
add_compile_definitions(GTEST_HAS_ABSL)

add_custom_target(echogTest
        COMMAND echo gtest_SOURCE_DIR=${gtest_SOURCE_DIR}
        COMMAND ls -l ${gtest_SOURCE_DIR}/include/gtest/gtest.h
        )
add_subdirectory(plugin)
add_subdirectory(sessionrunner)
