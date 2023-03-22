message("directory : " ${PROJECT_BINARY_DIR}/modules/Telemetry)
add_executable(telemetry main.cpp)
add_dependencies(telemetry telemetryimpl)
target_include_directories(telemetry  PUBLIC ${PROJECT_SOURCE_DIR}/modules/Telemetry)

target_link_libraries(telemetry telemetryimpl)

SET_TARGET_PROPERTIES(telemetry
        PROPERTIES
        INSTALL_RPATH "$ORIGIN/../lib64"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

install(TARGETS telemetry
        DESTINATION ${BASE_DIST_PREFIX}/files/base/bin)
