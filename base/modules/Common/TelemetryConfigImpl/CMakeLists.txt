add_library(telemetryconfigimpl SHARED
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

include_directories(telemetryconfigimpl BEFORE ${NLOHMANN_JSON_INCLUDE_DIR})

target_link_libraries(telemetryconfigimpl PUBLIC obfuscationimpl logging)

SET_TARGET_PROPERTIES(telemetryconfigimpl
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

install(TARGETS telemetryconfigimpl
        LIBRARY DESTINATION ${BASE_DIST_PREFIX}/files/base/lib64)

add_dependencies(telemetryconfigimpl copy_libs)