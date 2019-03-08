add_library(zmqwrapperapiimplobject OBJECT
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

target_include_directories(zmqwrapperapiimplobject PUBLIC ${ZeroMQ_INCLUDE_DIR} ${LOG4CPLUS_INCLUDE_DIR} ${CMAKE_SOURCE_DIR})

add_library(zmqwrapperapiimpl SHARED $<TARGET_OBJECTS:zmqwrapperapiimplobject> )
target_link_libraries(zmqwrapperapiimpl PUBLIC ${ZeroMQ_LIBRARY} ${LOG4CPLUS_LIBRARY} reactorimpl zeromqwrapperimpl)

SET_TARGET_PROPERTIES( zmqwrapperapiimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS zmqwrapperapiimpl
        LIBRARY DESTINATION files/base/lib64)