add_executable(tscheduler main.cpp)
target_include_directories(tscheduler PUBLIC ${PROJECT_SOURCE_DIR}/modules/TelemetryScheduler)

target_link_libraries(tscheduler telemetryschedulerimpl)

configure_file(tscheduler.json.in  tscheduler.json @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/tscheduler.json
        DESTINATION installer/plugins
        PERMISSIONS OWNER_READ)

SET_TARGET_PROPERTIES( tscheduler
        PROPERTIES INSTALL_RPATH "$ORIGIN/../lib64"
        BUILD_RPATH "${SUL_LIBRARY_DIRS}"
        )

install(TARGETS tscheduler
        DESTINATION files/base/bin)
