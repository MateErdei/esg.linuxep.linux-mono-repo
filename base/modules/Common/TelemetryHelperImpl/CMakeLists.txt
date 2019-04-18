add_library(telemetryhelperimpl SHARED
        TelemetryValue.cpp
        TelemetryValue.h
        TelemetrySerialiser.h
        TelemetrySerialiser.cpp
        TelemetryObject.h
        TelemetryObject.cpp
        Logger.h
        Logger.cpp)

target_include_directories(telemetryhelperimpl PUBLIC ../../Common ${LOG4CPLUS_INCLUDE_DIR} ${NLOHMANN_JSON_INCLUDE})

target_link_libraries(telemetryhelperimpl PUBLIC ${LOG4CPLUS_LIBRARY} logging)

SET_TARGET_PROPERTIES(telemetryhelperimpl PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS telemetryhelperimpl LIBRARY DESTINATION files/base/lib64)
