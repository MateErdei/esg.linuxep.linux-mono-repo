add_library(telemetryschedulerimpl SHARED
        Scheduler.cpp
        Scheduler.h
        PluginCallback.cpp
        PluginCallback.h
        SchedulerProcessor.cpp
        SchedulerProcessor.h
        SchedulerTask.h
        TaskQueue.cpp
        TaskQueue.h)

include_directories(telemetryschedulerimpl BEFORE ${LOG4CPLUS_INCLUDE_DIR})

target_link_libraries(telemetryschedulerimpl PUBLIC
        telemetryschedulerloggerimpl
        pluginapiimpl
        )

SET_TARGET_PROPERTIES(telemetryschedulerimpl
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

install(TARGETS telemetryschedulerimpl
        LIBRARY DESTINATION files/base/lib64)

add_dependencies(telemetryschedulerimpl copy_libs)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} --coverage")
