add_library(statusreceiverimpl SHARED
            ../PluginCommunication/IStatusReceiver.h
            StatusReceiverImpl.cpp
            StatusReceiverImpl.h
            StatusTask.cpp
            StatusTask.h)
target_include_directories(statusreceiverimpl PUBLIC  ../../Common ${LOG4CPLUS_INCLUDE_DIR})
target_link_libraries(statusreceiverimpl PUBLIC applicationconfigurationimpl taskqueueimpl statuscacheimpl managementagentloggerimpl)

SET_TARGET_PROPERTIES( statusreceiverimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS statusreceiverimpl
        LIBRARY DESTINATION ${BASE_DIST_PREFIX}/files/base/lib64)
