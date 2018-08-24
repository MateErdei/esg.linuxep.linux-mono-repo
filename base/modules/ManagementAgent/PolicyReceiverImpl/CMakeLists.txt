cmake_minimum_required(VERSION 3.5)
add_library(policyreceiverimpl SHARED
        PolicyReceiverImpl.cpp
        PolicyReceiverImpl.h
        )
target_include_directories(policyreceiverimpl PUBLIC  ../../Common filesystemimpl ../PluginCommunication ${LOG4CPLUS_INCLUDE_DIR})
target_link_libraries(policyreceiverimpl PUBLIC threads filesystemimpl pluginapiimpl plugincommunicationimpl utilityimpl managementagentloggerimpl mcsrouterplugincommunicationimpl taskqueueimpl)

SET_TARGET_PROPERTIES( policyreceiverimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS policyreceiverimpl
        LIBRARY DESTINATION files/base/lib64)