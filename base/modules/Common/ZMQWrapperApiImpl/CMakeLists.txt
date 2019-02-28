add_library(zmqwrapperapiimpl OBJECT
        ContextImpl.cpp
        ContextImpl.h
        LoggingProxyImpl.cpp LoggingProxyImpl.h)

target_include_directories(zmqwrapperapiimpl PUBLIC ${ZeroMQ_INCLUDE_DIR})
target_link_libraries(zmqwrapperapiimpl PUBLIC ${ZeroMQ_LIBRARY} )

SET_TARGET_PROPERTIES( zmqwrapperapiimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS zmqwrapperapiimpl
        LIBRARY DESTINATION files/base/lib64)