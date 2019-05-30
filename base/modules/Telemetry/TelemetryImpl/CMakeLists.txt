add_library(telemetryimpl SHARED
        Telemetry.cpp
        Telemetry.h
        ISystemTelemetryCollector.h
        SystemTelemetryCollectorImpl.cpp
        SystemTelemetryCollectorImpl.h
        SystemTelemetryConfig.cpp
        SystemTelemetryConfig.h
        SystemTelemetryReporter.cpp
        SystemTelemetryReporter.h
        TelemetryProcessor.cpp
        TelemetryProcessor.h
        ITelemetryProvider.h
        ../TelemetryConfig/TelemetryConfigSerialiser.h
        ../TelemetryConfig/TelemetryConfigSerialiser.cpp
        ../TelemetryConfig/Proxy.h
        ../TelemetryConfig/Proxy.cpp
        ../TelemetryConfig/MessageRelay.h
        ../TelemetryConfig/MessageRelay.cpp
        ../TelemetryConfig/Config.h
        ../TelemetryConfig/Config.cpp
        )

include_directories(telemetryimpl BEFORE ${CURL_INCLUDE_DIR} ${LOG4CPLUS_INCLUDE_DIR} ../../../thirdparty/nlohmann-json)

target_link_libraries(telemetryimpl PUBLIC
        telemetryloggerimpl
        httpsenderimpl
        telemetryexeconfigimpl
        telemetryhelperimpl
        processimpl
        threads
        )

SET_TARGET_PROPERTIES(telemetryimpl
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

install(TARGETS telemetryimpl
        LIBRARY DESTINATION files/base/lib64)

add_dependencies(telemetryimpl copy_libs)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} --coverage")