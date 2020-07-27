include(GoogleTest)

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

SophosAddTest(TestFileWalker
        TestFileWalker.cpp
        TestFileWalkerBackTrackProtection.cpp
        PROJECTS filewalker
        INC_DIRS
        ${CMAKE_SOURCE_DIR}/modules
        )
SET_TARGET_PROPERTIES(TestFileWalker
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )
