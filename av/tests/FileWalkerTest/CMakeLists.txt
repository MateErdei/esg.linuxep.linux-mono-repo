
add_executable(file_walker_experiment
        FileWalkerExperiment.cpp)

target_include_directories(file_walker_experiment PRIVATE ${CMAKE_SOURCE_DIR}/modules)

target_link_libraries(file_walker_experiment PRIVATE filewalker stdc++fs)



SET_TARGET_PROPERTIES( file_walker_experiment
        PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib64:$ORIGIN/../../../base/lib64"
        )
