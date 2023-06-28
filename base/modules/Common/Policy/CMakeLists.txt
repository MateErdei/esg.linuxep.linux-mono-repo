include(proto/CMakeLists.txt)

add_library(policyobject OBJECT
        ALCPolicy.cpp
        ALCPolicy.h
        Credentials.cpp
        Credentials.h
        Logger.cpp
        Logger.h
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


include_directories(policyobject BEFORE ${CMAKE_CURRENT_BINARY_DIR} ${LOG4CPLUS_INCLUDE_DIR} ${NLOHMANN_JSON_INCLUDE_DIR})
target_link_libraries(policyobject INTERFACE
        obfuscationimpl
        osutilitiesimpl
        protobufutil
        xmlutilities
        )

add_library(policy SHARED)
include_directories(policy BEFORE ${CMAKE_CURRENT_BINARY_DIR} ${LOG4CPLUS_INCLUDE_DIR} ${NLOHMANN_JSON_INCLUDE_DIR})
target_link_libraries(policy PRIVATE
        policyobject
        obfuscationimpl
        osutilitiesimpl
        protobufutil
        xmlutilities
        )

SET_TARGET_PROPERTIES(policy
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

install(TARGETS policy
        LIBRARY DESTINATION ${BASE_DIST_PREFIX}/files/base/lib64)

add_dependencies(policy copy_libs)
