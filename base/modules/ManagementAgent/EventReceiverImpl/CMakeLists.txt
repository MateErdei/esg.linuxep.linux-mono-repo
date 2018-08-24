add_library(eventreceiverimpl SHARED
        EventReceiverImpl.cpp EventReceiverImpl.h
        ../PluginCommunication/IEventReceiver.h
        EventTask.cpp
        EventTask.h)
target_include_directories(eventreceiverimpl PUBLIC  ../../Common ${LOG4CPLUS_INCLUDE_DIR})
target_link_libraries(eventreceiverimpl PUBLIC applicationconfigurationimpl managementagentloggerimpl taskqueueimpl)

SET_TARGET_PROPERTIES( eventreceiverimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS eventreceiverimpl
        LIBRARY DESTINATION files/base/lib64)
