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
        PluginTelemetryReporter.cpp
        PluginTelemetryReporter.h
        BaseTelemetryReporter.cpp
        BaseTelemetryReporter.h
        TelemetryUtils.cpp
        TelemetryUtils.h
        ../../Common/CurlWrapper/CurlWrapper.h
        ../../Common/CurlWrapper/CurlWrapper.cpp
        )

include_directories(telemetryimpl BEFORE ${CURL_INCLUDE_DIR} ${LOG4CPLUS_INCLUDE_DIR} ${NLOHMANN_JSON_INCLUDE_DIR} ../../ManagementAgent ${BOOST_INCLUDE_DIR} ${CMAKE_SOURCE_DIR})

target_link_libraries(telemetryimpl PUBLIC
        HttpRequestsImpl
        plugincommunicationimpl
        processimpl
        telemetryconfigimpl
        telemetryhelperimpl
        telemetryloggerimpl
        threads
        xmlutilities
        )

SET_TARGET_PROPERTIES(telemetryimpl
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

install(TARGETS telemetryimpl
        LIBRARY DESTINATION ${BASE_DIST_PREFIX}/files/base/lib64)

add_dependencies(telemetryimpl copy_libs)
