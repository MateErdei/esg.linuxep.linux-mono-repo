message("directory : " ${PROJECT_BINARY_DIR}/modules/Telemetry)
add_executable(Telemetry main.cpp)
add_dependencies(Telemetry telemetryimpl)
target_include_directories(Telemetry  PUBLIC ${PROJECT_SOURCE_DIR}/modules/Telemetry)

target_link_libraries(Telemetry  telemetryimpl)

SET_TARGET_PROPERTIES( Telemetry
        PROPERTIES
        INSTALL_RPATH "$ORIGIN/../lib64"
        BUILD_RPATH "${SUL_LIBRARY_DIRS}"
        )

install(TARGETS Telemetry
        DESTINATION files/base/bin)
