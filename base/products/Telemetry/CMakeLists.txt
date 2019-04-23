message("directory : " ${PROJECT_BINARY_DIR}/modules/Telemetry)
add_executable(telemetry main.cpp)
add_dependencies(telemetry telemetryimpl)
target_include_directories(telemetry  PUBLIC ${PROJECT_SOURCE_DIR}/modules/Telemetry ${CURL_INCLUDE_DIR})

target_link_libraries(telemetry telemetryimpl telemetryloggerimpl)

SET_TARGET_PROPERTIES(telemetry
        PROPERTIES
        INSTALL_RPATH "$ORIGIN/../lib64"
        BUILD_RPATH "${SUL_LIBRARY_DIRS}"
        )

install(TARGETS telemetry
        DESTINATION files/base/bin)
