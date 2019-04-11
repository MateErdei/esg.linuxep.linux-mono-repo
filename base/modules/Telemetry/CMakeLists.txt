cmake_minimum_required(VERSION 3.8)

add_library(telemetryimpl SHARED
        Telemetry.cpp
        Telemetry.h
        Logger.cpp
        Logger.h)


include_directories(telemetryimpl BEFORE ${LOG4CPLUS_INCLUDE_DIR})

target_link_libraries(telemetryimpl PUBLIC
        ${LOG4CPLUS_LIBRARY}
        logging
        )

install(TARGETS telemetryimpl
        LIBRARY DESTINATION files/base/lib64)
