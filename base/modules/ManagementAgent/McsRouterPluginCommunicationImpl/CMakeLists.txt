cmake_minimum_required(VERSION 3.5)
add_library(mcsrouterplugincommunicationimpl SHARED
        TaskDirectoryListener.cpp
        TaskDirectoryListener.h
        PolicyTask.cpp
        PolicyTask.h
        ActionTask.h
        ActionTask.cpp)
target_include_directories(mcsrouterplugincommunicationimpl PUBLIC  ../../Common filesystemimpl ../PluginCommunication ${LOG4CPLUS_INCLUDE_DIR})
target_link_libraries(mcsrouterplugincommunicationimpl PUBLIC threads filesystemimpl pluginapiimpl plugincommunicationimpl managementagentloggerimpl)

SET_TARGET_PROPERTIES( mcsrouterplugincommunicationimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS mcsrouterplugincommunicationimpl
        LIBRARY DESTINATION files/base/lib64)