add_library(telemetryhelperimplobject OBJECT
        TelemetryHelper.h
        TelemetryHelper.cpp
        TelemetryValue.cpp
        TelemetryValue.h
        TelemetrySerialiser.h
        TelemetrySerialiser.cpp
        TelemetryObject.h
        TelemetryObject.cpp
        Logger.h
        Logger.cpp
        TelemetryJsonToMap.cpp
        TelemetryJsonToMap.h)

target_include_directories(telemetryhelperimplobject PUBLIC ../../Common ${LOG4CPLUS_INCLUDE_DIR} ${NLOHMANN_JSON_INCLUDE_DIR})

add_library(telemetryhelperimpl SHARED $<TARGET_OBJECTS:telemetryhelperimplobject>)

target_link_libraries(telemetryhelperimpl PUBLIC ${LOG4CPLUS_LIBRARY} logging)

SET_TARGET_PROPERTIES(telemetryhelperimpl PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS telemetryhelperimpl LIBRARY DESTINATION ${BASE_DIST_PREFIX}/files/base/lib64)
