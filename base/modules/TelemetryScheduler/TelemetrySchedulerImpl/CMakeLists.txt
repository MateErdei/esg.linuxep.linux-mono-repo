add_library(telemetryschedulerimpl SHARED
        Scheduler.cpp
        Scheduler.h
        PluginCallback.cpp
        PluginCallback.h
        SchedulerProcessor.cpp
        SchedulerProcessor.h
        TaskQueue.cpp
        TaskQueue.h
        SchedulerStatus.cpp
        SchedulerStatusSerialiser.cpp
        SchedulerStatusSerialiser.h
        SleepyThread.cpp
        SleepyThread.h)

include_directories(telemetryschedulerimpl BEFORE ${LOG4CPLUS_INCLUDE_DIR} ../../../thirdparty/nlohmann-json)

target_link_libraries(telemetryschedulerimpl PUBLIC
        telemetryschedulerloggerimpl
        pluginapiimpl
        processimpl
        telemetryconfigimpl
        utilityimpl
        osutilitiesimpl
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
