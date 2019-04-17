add_library(telemetryimpl

        TelemetryDictionary.cpp
        TelemetryDictionary.h
        TelemetryValue.cpp
        TelemetryValue.h
        TelemetryNode.cpp
        TelemetryNode.h
        TelemetrySerialiser.h
        TelemetrySerialiser.cpp
        Logger.h
        Logger.cpp)

target_include_directories(telemetryimpl PUBLIC ../../Common ${LOG4CPLUS_INCLUDE_DIR} ${NLOHMANN_JSON_INCLUDE})

target_link_libraries(telemetryimpl PUBLIC ${LOG4CPLUS_LIBRARY} logging)

SET_TARGET_PROPERTIES(telemetryimpl PROPERTIES INSTALL_RPATH "$ORIGIN")

#install(TARGETS telemetryimpl LIBRARY DESTINATION files/base/lib64)
