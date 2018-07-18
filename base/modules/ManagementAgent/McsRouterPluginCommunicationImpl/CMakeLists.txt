cmake_minimum_required(VERSION 3.5)
add_library(mcsrouterplugincommunicationimpl SHARED
        TaskDirectoryListener.cpp
        TaskDirectoryListener.h
        TaskQueueProcessor.cpp
        TaskQueueProcessor.h
        TaskQueue.cpp
        TaskQueue.h
        TaskQueueProcessorThread.cpp
        TaskQueueProcessorThread.h
        Logger.h)
target_include_directories(mcsrouterplugincommunicationimpl PUBLIC  ../../Common filesystemimpl ../PluginCommunication)
target_link_libraries(mcsrouterplugincommunicationimpl PUBLIC threads filesystemimpl pluginapiimpl plugincommunicationimpl)
install(TARGETS mcsrouterplugincommunicationimpl
        LIBRARY DESTINATION files/base/lib64)