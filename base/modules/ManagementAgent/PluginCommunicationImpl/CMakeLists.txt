cmake_minimum_required(VERSION 3.5)
add_library(plugincommunicationimpl SHARED
        ../PluginCommunication/IPluginServerCallback.h
        ../PluginCommunication/IPluginProxy.h
        ../PluginCommunication/IPluginManager.h
        ../PluginCommunication/IPluginCommunicationException.h
        PluginServerCallbackHandler.h
        PluginServerCallbackHandler.cpp
        PluginManager.cpp
        PluginManager.h
        PluginProxy.cpp
        PluginProxy.h
        PluginServerCallback.cpp
        PluginServerCallback.h
        ../../Common/PluginApi/StatusInfo.h)
target_include_directories(plugincommunicationimpl PUBLIC  ../../Common ../PluginCommunication)
target_link_libraries(plugincommunicationimpl PUBLIC threads pluginapiimpl)
add_dependencies(plugincommunicationimpl pluginapiimpl)