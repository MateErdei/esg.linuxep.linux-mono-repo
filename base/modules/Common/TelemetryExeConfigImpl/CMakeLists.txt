add_library(telemetryexeconfigimpl SHARED
        Constants.h
        Serialiser.h
        Serialiser.cpp
        Config.h
        Config.cpp
        Proxy.h
        Proxy.cpp
        MessageRelay.h
        MessageRelay.cpp
        )

include_directories(telemetryexeconfigimpl BEFORE ../../../thirdparty/nlohmann-json)

target_link_libraries(telemetryexeconfigimpl PUBLIC obfuscationimpl)

SET_TARGET_PROPERTIES(telemetryexeconfigimpl
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

install(TARGETS telemetryexeconfigimpl
        LIBRARY DESTINATION files/base/lib64)

add_dependencies(telemetryexeconfigimpl copy_libs)