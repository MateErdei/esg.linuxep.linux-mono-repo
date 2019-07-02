cmake_minimum_required(VERSION 3.5)
add_library(plugincommunicationimpl SHARED
        ../PluginCommunication/IEventReceiver.h
        ../PluginCommunication/IPluginServerCallback.h
        ../PluginCommunication/IStatusReceiver.h
        ../PluginCommunication/IPluginManager.h
        PluginServerCallbackHandler.h
        PluginServerCallbackHandler.cpp
        PluginManager.cpp
        PluginManager.h
        PluginServerCallback.cpp
        PluginServerCallback.h
        )
target_include_directories(plugincommunicationimpl PUBLIC  ${CMAKE_SOURCE_DIR}/modules ${LOG4CPLUS_INCLUDE_DIR})
target_link_libraries(plugincommunicationimpl PUBLIC threads pluginapiimpl applicationconfigurationimpl pluginregistryimpl plugincommunicationimplcommon managementagentloggerimpl)

SET_TARGET_PROPERTIES( plugincommunicationimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

add_dependencies(plugincommunicationimpl pluginapiimpl)
install(TARGETS plugincommunicationimpl
        LIBRARY DESTINATION files/base/lib64)
