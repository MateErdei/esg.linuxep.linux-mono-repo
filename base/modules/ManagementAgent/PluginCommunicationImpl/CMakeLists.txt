cmake_minimum_required(VERSION 3.5)
add_library(plugincommunicationimpl SHARED
        ../../Common/PluginApi/StatusInfo.h
        ../PluginCommunication/IPluginServerCallback.h
        ../PluginCommunication/IStatusReceiver.h
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
        Logger.h
        AppIdCollection.cpp
        AppIdCollection.h)
target_include_directories(plugincommunicationimpl PUBLIC  ../../Common ../PluginCommunication)
target_link_libraries(plugincommunicationimpl PUBLIC threads pluginapiimpl applicationconfigurationimpl)
add_dependencies(plugincommunicationimpl pluginapiimpl)
install(TARGETS plugincommunicationimpl
        LIBRARY DESTINATION files/base/lib64)
