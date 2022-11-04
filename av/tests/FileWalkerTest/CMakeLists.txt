add_executable(file_walker_experiment
        FileWalkerExperiment.cpp)

target_include_directories(file_walker_experiment PRIVATE ${CMAKE_SOURCE_DIR}/modules)

target_link_libraries(file_walker_experiment PRIVATE filewalker
        ${STD_FILESYSTEM_IF_REQUIRED}
        )

SET_TARGET_PROPERTIES( file_walker_experiment
        PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib64:$ORIGIN/../../../base/lib64"
        )

add_executable(file_walker_stress_test
        FileWalkerStressTest.cpp)
target_include_directories(file_walker_stress_test PRIVATE ${CMAKE_SOURCE_DIR}/modules)
target_link_libraries(file_walker_stress_test PRIVATE filewalker pthread
        )
SET_TARGET_PROPERTIES(file_walker_stress_test
        PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib64:$ORIGIN/../../../base/lib64"
        )

SophosAddTest(TestFileWalker
        ../common/LogInitializedTests.cpp
        TestFileWalker.cpp
        TestFileWalkerRecursion.cpp
        PROJECTS filewalker
        LIBS ${log4cpluslib}
        )

