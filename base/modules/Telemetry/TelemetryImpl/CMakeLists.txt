add_library(telemetryimpl SHARED
        Telemetry.cpp
        Telemetry.h)

include_directories(telemetryimpl BEFORE ${LOG4CPLUS_INCLUDE_DIR} ${CURL_INCLUDE_DIR})

target_link_libraries(telemetryimpl PUBLIC
        ${LOG4CPLUS_LIBRARY}
        telemetryloggerimpl
        httpsenderimpl
        )

SET_TARGET_PROPERTIES(telemetryimpl
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

install(TARGETS telemetryimpl
        LIBRARY DESTINATION files/base/lib64)

add_dependencies(telemetryimpl copy_libs)