add_executable(UpdateScheduler main.cpp)
target_include_directories(UpdateScheduler PUBLIC ${PROJECT_SOURCE_DIR}/modules/UpdateSchedulerImpl)

target_link_libraries(UpdateScheduler PUBLIC updateschedulerimpl)

configure_file(updatescheduler.json.in  updatescheduler.json @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/updatescheduler.json
        DESTINATION installer/plugins
        PERMISSIONS OWNER_READ )

SET_TARGET_PROPERTIES( UpdateScheduler
        PROPERTIES INSTALL_RPATH "$ORIGIN/../lib64"
        BUILD_RPATH "${SUL_LIBRARY_DIRS}"
        )

install(TARGETS UpdateScheduler
        DESTINATION files/base/bin)