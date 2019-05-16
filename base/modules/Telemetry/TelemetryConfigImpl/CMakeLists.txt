add_library(telemetryconfigimpl SHARED
        Constants.h
        TelemetryConfigSerialiser.h
        TelemetryConfigSerialiser.cpp
        Config.h
        Config.cpp
        Proxy.h
        Proxy.cpp
        MessageRelay.h
        MessageRelay.cpp
        )

include_directories(telemetryconfigimpl BEFORE ../../../thirdparty/nlohmann-json)

target_link_libraries(telemetryconfigimpl PUBLIC obfuscationimpl)

SET_TARGET_PROPERTIES(telemetryconfigimpl
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

install(TARGETS telemetryconfigimpl
        LIBRARY DESTINATION files/base/lib64)

add_dependencies(telemetryconfigimpl copy_libs)