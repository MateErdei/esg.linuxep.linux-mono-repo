add_library(mcsimpl SHARED
        IAdapter.h
        AgentAdapter.h
        AgentAdapter.cpp
        MCSHttpClient.cpp
        MCSHttpClient.h
        Logger.cpp
        Logger.h
        MCSApiCalls.cpp
        MCSApiCalls.h
        Config.h
        MessageRelay.h
        ConfigOptions.h
        ConfigOptions.cpp
        )

include_directories(mcsimpl BEFORE ${LOG4CPLUS_INCLUDE_DIR} ${NLOHMANN_JSON_INCLUDE_DIR})
target_link_libraries(mcsimpl PUBLIC HttpRequestsImpl obfuscationimpl osutilitiesimpl xmlutilities)

SET_TARGET_PROPERTIES(mcsimpl
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

install(TARGETS mcsimpl
        LIBRARY DESTINATION ${BASE_DIST_PREFIX}/files/base/lib64)

add_dependencies(mcsimpl copy_libs)
