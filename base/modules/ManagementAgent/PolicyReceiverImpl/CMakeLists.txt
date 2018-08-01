cmake_minimum_required(VERSION 3.5)
add_library(policyreceiverimpl SHARED
        PolicyReceiverImpl.cpp
        PolicyReceiverImpl.h
        Logger.h)
target_include_directories(policyreceiverimpl PUBLIC  ../../Common filesystemimpl ../PluginCommunication)
target_link_libraries(policyreceiverimpl PUBLIC threads filesystemimpl pluginapiimpl plugincommunicationimpl utilityimpl)
install(TARGETS policyreceiverimpl
        LIBRARY DESTINATION files/base/lib64)