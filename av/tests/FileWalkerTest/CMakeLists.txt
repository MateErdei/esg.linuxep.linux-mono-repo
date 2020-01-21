
add_executable(file_walker_experiment
        FileWalkerExperiment.cpp)

target_include_directories(file_walker_experiment PRIVATE ${CMAKE_SOURCE_DIR}/modules)

target_link_libraries(file_walker_experiment PRIVATE filewalker
        $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0>>:stdc++fs>
        )



SET_TARGET_PROPERTIES( file_walker_experiment
        PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib64:$ORIGIN/../../../base/lib64"
        )

add_executable(TestFileWalkerBackTrackProtection
        TestFileWalkerBackTrackProtection.cpp)
target_include_directories(TestFileWalkerBackTrackProtection PRIVATE ${CMAKE_SOURCE_DIR}/modules)
target_link_libraries(TestFileWalkerBackTrackProtection PRIVATE filewalker)
SET_TARGET_PROPERTIES(TestFileWalkerBackTrackProtection
        PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib64:$ORIGIN/../../../base/lib64"
        )
