# Generate proto files
INCLUDE_DIRECTORIES(SYSTEM BEFORE PUBLIC ${PROTOBUF_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
set(ENV{LD_LIBRARY_PATH} ${Protobuf_LD_LIBRARY_PATH})
#set(Protobuf_PROTOC_EXECUTABLE ${CMAKE_BINARY_DIR}/protoc.sh)
if (EXISTS ${PROTOBUF_PROTOC_EXECUTABLE})
else ()
    message(FATAL_ERROR "Could not find PROTOBUF Compiler")
endif ()
PROTOBUF_GENERATE_CPP(CONFIG_SETTINGS_PROTO_SRC CONFIG_SETTINGS_PROTO_H ConfigurationSettings.proto)

####

add_library(policyobject OBJECT
        ALCPolicy.cpp
        ALCPolicy.h
        Credentials.cpp
        Credentials.h
        ESMVersion.h
        Logger.cpp
        Logger.h
        MCSPolicy.cpp
        MCSPolicy.h
        PolicyHelper.h
        PolicyHelper.cpp
        PolicyParseException.h
        ProductSubscription.h
        Proxy.cpp
        Proxy.h
        ProxyCredentials.cpp
        ProxyCredentials.h
        SerialiseUpdateSettings.cpp
        SerialiseUpdateSettings.h
        UpdateSettings.cpp
        UpdateSettings.h
        WeekDayAndTimeForDelay.h
        ${CONFIG_SETTINGS_PROTO_SRC}
        ${CONFIG_SETTINGS_PROTO_H}
)


include_directories(policyobject BEFORE ${CMAKE_BINARY_DIR}/modules ${CMAKE_CURRENT_BINARY_DIR} ${LOG4CPLUS_INCLUDE_DIR} ${NLOHMANN_JSON_INCLUDE_DIR} ${SOPHLIB_INCLUDE})
target_link_libraries(policyobject PRIVATE
        obfuscationimpl
        osutilitiesimpl
        protobufutil
        xmlutilities
        sophlib_hostname
        )

add_library(policy SHARED)
include_directories(policy BEFORE ${CMAKE_CURRENT_BINARY_DIR} ${LOG4CPLUS_INCLUDE_DIR} ${NLOHMANN_JSON_INCLUDE_DIR} ${SOPHLIB_INCLUDE})
target_link_libraries(policy PRIVATE
        policyobject
        obfuscationimpl
        osutilitiesimpl
        protobufutil
        xmlutilities
        sophlib_hostname
        )

SET_TARGET_PROPERTIES(policy
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

install(TARGETS policy
        LIBRARY DESTINATION ${BASE_DIST_PREFIX}/files/base/lib64)

add_dependencies(policy copy_libs)
