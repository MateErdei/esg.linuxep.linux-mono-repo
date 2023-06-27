
add_library(policyobject OBJECT
        ALCPolicy.cpp
        ALCPolicy.h
        Credentials.h
        Logger.cpp
        Logger.h
        PolicyParseException.h
        ProductSubscription.h
        Proxy.cpp
        Proxy.h
        ProxyCredentials.cpp
        ProxyCredentials.h
        UpdateSettings.h
        WeekDayAndTimeForDelay.h
        )

add_library(policy SHARED
        )

include_directories(policy BEFORE ${LOG4CPLUS_INCLUDE_DIR} ${NLOHMANN_JSON_INCLUDE_DIR})
target_link_libraries(policy PRIVATE policyobject obfuscationimpl xmlutilities osutilitiesimpl)

SET_TARGET_PROPERTIES(policy
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

install(TARGETS policy
        LIBRARY DESTINATION ${BASE_DIST_PREFIX}/files/base/lib64)

add_dependencies(policy copy_libs)
