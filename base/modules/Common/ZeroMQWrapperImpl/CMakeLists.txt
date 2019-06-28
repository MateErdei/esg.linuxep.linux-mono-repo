add_library(zeromqwrapperimplobject OBJECT
        ContextHolder.cpp ContextHolder.h
        PollerImpl.cpp PollerImpl.h
        ProxyImpl.cpp ProxyImpl.h
        SocketHolder.cpp SocketHolder.h
        SocketImpl.cpp SocketImpl.h
        SocketPublisherImpl.cpp SocketPublisherImpl.h
        SocketReplierImpl.cpp SocketReplierImpl.h
        SocketRequesterImpl.cpp SocketRequesterImpl.h
        SocketSubscriberImpl.cpp SocketSubscriberImpl.h
        SocketUtil.cpp SocketUtil.h
        ZeroMQWrapperException.h
        ZeroMQTimeoutException.h
        ../Exceptions/IException.h
        ../Exceptions/Print.h
        ../ZeroMQWrapper/IHasFD.h
        ../ZeroMQWrapper/IReadable.h
        ../ZeroMQWrapper/IReadWrite.h
        ../ZeroMQWrapper/ISocketPublisher.h
        ../ZeroMQWrapper/ISocketReplier.h
        ../ZeroMQWrapper/ISocketRequester.h
        ../ZeroMQWrapper/ISocketSetup.h
        ../ZeroMQWrapper/ISocketSubscriber.h
        ../ZeroMQWrapper/IIPCException.h
        ../ZeroMQWrapper/IIPCTimeoutException.h
        ../ZeroMQWrapper/IPoller.h
        ../ZeroMQWrapper/IProxy.h
        ../ZeroMQWrapper/IWritable.h
        )
target_include_directories(zeromqwrapperimplobject PUBLIC ${ZeroMQ_INCLUDE_DIR})
add_library(zeromqwrapperimpl SHARED $<TARGET_OBJECTS:zeromqwrapperimplobject> )
target_link_libraries(zeromqwrapperimpl PUBLIC ${ZeroMQ_LIBRARY} threads )

SET_TARGET_PROPERTIES( zeromqwrapperimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS zeromqwrapperimpl
        LIBRARY DESTINATION files/base/lib64)
