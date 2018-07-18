add_library(statusreceiverimpl SHARED
            StatusCache.h
            StatusCache.cpp
            StatusReceiverImpl.cpp
            StatusReceiverImpl.h
            ../PluginCommunication/IStatusReceiver.h)
target_include_directories(statusreceiverimpl PUBLIC  ../../Common )
target_link_libraries(statusreceiverimpl PUBLIC)
install(TARGETS statusreceiverimpl
        LIBRARY DESTINATION files/base/lib64)
