add_library(telemetryloggerimpl SHARED
        Logger.h
        Logger.cpp)
target_include_directories(telemetryloggerimpl PUBLIC  ../../Common ${LOG4CPLUS_INCLUDE_DIR})
target_link_libraries(telemetryloggerimpl PUBLIC
        ${LOG4CPLUS_LIBRARY}
        logging
        )

SET_TARGET_PROPERTIES( telemetryloggerimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS telemetryloggerimpl
        LIBRARY DESTINATION files/base/lib64)
