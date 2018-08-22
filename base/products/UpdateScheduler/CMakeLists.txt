add_executable(UpdateScheduler main.cpp)
target_include_directories(UpdateScheduler PUBLIC ${PROJECT_SOURCE_DIR}/modules/UpdateSchedulerImpl)

target_link_libraries(UpdateScheduler PUBLIC updateschedulerimpl)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/updatescheduler.json
        DESTINATION installer/plugins
        PERMISSIONS OWNER_READ )

SET_TARGET_PROPERTIES( UpdateScheduler
        PROPERTIES INSTALL_RPATH "$ORIGIN/../lib64")

install(TARGETS UpdateScheduler
        DESTINATION files/base/bin)