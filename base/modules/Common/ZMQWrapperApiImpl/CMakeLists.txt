add_library(zmqwrapperapiimpl OBJECT
        ContextImpl.cpp
        ContextImpl.h
        LoggingProxyImpl.cpp
        LoggingProxyImpl.h
        Logger.cpp
        Logger.h
        ../ZMQWrapperApi/IContext.h
        ../ZMQWrapperApi/IContextPtr.h
        ../ZMQWrapperApi/IContextSharedPtr.h
        )

target_include_directories(zmqwrapperapiimpl PUBLIC ${ZeroMQ_INCLUDE_DIR} ${LOG4CPLUS_INCLUDE_DIR})
target_link_libraries(zmqwrapperapiimpl PUBLIC ${ZeroMQ_LIBRARY} ${LOG4CPLUS_LIBRARY} reactorimpl zeromqwrapperimpl)

SET_TARGET_PROPERTIES( zmqwrapperapiimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS zmqwrapperapiimpl
        LIBRARY DESTINATION files/base/lib64)