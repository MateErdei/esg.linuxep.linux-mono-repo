add_library(telemetryimpl SHARED
        Telemetry.cpp
        Telemetry.h
        Logger.cpp
        Logger.h
        ISystemTelemetryCollector.h
        SystemTelemetryCollectorImpl.cpp
        SystemTelemetryCollectorImpl.h
        SystemTelemetryConfig.cpp
        SystemTelemetryConfig.h)

include_directories(telemetryimpl BEFORE ${LOG4CPLUS_INCLUDE_DIR})

target_link_libraries(telemetryimpl PUBLIC
        ${LOG4CPLUS_LIBRARY}
        logging
        processimpl
        threads
        )

install(TARGETS telemetryimpl
        LIBRARY DESTINATION files/base/lib64)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} --coverage")
