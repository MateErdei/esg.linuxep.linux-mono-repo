cmake_minimum_required(VERSION 3.5)

add_library(plugincommunicationimplcommon SHARED
        ../PluginApi/StatusInfo.h
        ../PluginCommunication/IPluginProxy.h
        ../PluginCommunication/IPluginCommunicationException.h
        PluginProxy.cpp
        PluginProxy.h
        AppIdCollection.cpp
        AppIdCollection.h
        )

target_include_directories(plugincommunicationimplcommon PUBLIC  ${CMAKE_SOURCE_DIR}/modules ../PluginCommunication ${LOG4CPLUS_INCLUDE_DIR})

target_link_libraries(plugincommunicationimplcommon PUBLIC pluginapiimpl utilityimpl)

SET_TARGET_PROPERTIES( plugincommunicationimplcommon
        PROPERTIES INSTALL_RPATH "$ORIGIN")

add_dependencies(plugincommunicationimplcommon pluginapiimpl)

install(TARGETS plugincommunicationimplcommon
        LIBRARY DESTINATION files/base/lib64)
