cmake_minimum_required(VERSION 3.5)
add_library(mcsrouterplugincommunicationimpl SHARED
        TaskDirectoryListener.cpp
        TaskDirectoryListener.h
        Logger.h PolicyTask.cpp PolicyTask.h PolicyAndActionListener.cpp PolicyAndActionListener.h ActionTask.h ActionTask.cpp)
target_include_directories(mcsrouterplugincommunicationimpl PUBLIC  ../../Common filesystemimpl ../PluginCommunication)
target_link_libraries(mcsrouterplugincommunicationimpl PUBLIC threads filesystemimpl pluginapiimpl plugincommunicationimpl)
install(TARGETS mcsrouterplugincommunicationimpl
        LIBRARY DESTINATION files/base/lib64)