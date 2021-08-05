
add_library(pluginapiobjects OBJECT
        ../PluginApi/IBaseServiceApi.h
        ../PluginApi/ApiException.h
        ../PluginApi/NoPolicyAvailableException.h
        ../PluginApi/IPluginResourceManagement.h
        ../PluginApi/IPluginCallbackApi.h
        ../PluginApi/StatusInfo.h
        ../PluginApi/Logger.cpp
        ../PluginApi/Logger.h
        BaseServiceAPI.cpp
        BaseServiceAPI.h
        PluginCallBackHandler.cpp
        PluginCallBackHandler.h
        PluginResourceManagement.cpp
        PluginResourceManagement.h
        Logger.cpp
        Logger.h
        ../PluginApi/ErrorCodes.h
        ../PluginApi/NoPolicyAvailableException.cpp
        ../PluginApi/NoACKReplyException.h)

target_include_directories(pluginapiobjects PUBLIC ../PluginApi ../../Common ${LOG4CPLUS_INCLUDE_DIR})
add_library(pluginapiimpl SHARED $<TARGET_OBJECTS:pluginapiobjects>)

target_link_libraries(pluginapiimpl PUBLIC threads pluginprotocol ${LOG4CPLUS_LIBRARY} zmqwrapperapiimpl telemetryhelperimpl logging)

SET_TARGET_PROPERTIES(pluginapiimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS pluginapiimpl
        LIBRARY DESTINATION files/base/lib64)

