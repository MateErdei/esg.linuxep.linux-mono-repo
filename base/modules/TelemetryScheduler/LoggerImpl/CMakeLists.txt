add_library(telemetryschedulerloggerimpl SHARED
        Logger.h
        Logger.cpp)
target_include_directories(telemetryschedulerloggerimpl PUBLIC ../../Common ${LOG4CPLUS_INCLUDE_DIR})
target_link_libraries(telemetryschedulerloggerimpl PUBLIC
        ${LOG4CPLUS_LIBRARY}
        logging
        )

SET_TARGET_PROPERTIES(telemetryschedulerloggerimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS telemetryschedulerloggerimpl
        LIBRARY DESTINATION files/base/lib64)
