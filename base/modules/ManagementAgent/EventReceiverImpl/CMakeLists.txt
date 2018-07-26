add_library(eventreceiverimpl SHARED
        EventReceiverImpl.cpp EventReceiverImpl.h
        ../PluginCommunication/IEventReceiver.h
        EventTask.cpp
        EventTask.h)
target_include_directories(eventreceiverimpl PUBLIC  ../../Common )
target_link_libraries(eventreceiverimpl PUBLIC)
install(TARGETS eventreceiverimpl
        LIBRARY DESTINATION files/base/lib64)
