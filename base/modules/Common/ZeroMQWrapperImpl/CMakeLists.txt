add_library(zeromqwrapperimplobject OBJECT
        ContextHolder.cpp ContextHolder.h
        PollerFactory.cpp PollerFactory.h
        PollerImpl.cpp PollerImpl.h
        ProxyImpl.cpp ProxyImpl.h
        SocketHolder.cpp SocketHolder.h
        SocketImpl.cpp SocketImpl.h
        SocketPublisherImpl.cpp SocketPublisherImpl.h
        SocketReplierImpl.cpp SocketReplierImpl.h
        SocketRequesterImpl.cpp SocketRequesterImpl.h
        SocketSubscriberImpl.cpp SocketSubscriberImpl.h
        SocketUtil.cpp SocketUtil.h
        ZeroMQTimeoutException.h
        ZeroMQWrapperException.h
        ../Exceptions/IException.h
        ../Exceptions/Print.h
        ../ZeroMQWrapper/IHasFD.h
        ../ZeroMQWrapper/IIPCException.h
        ../ZeroMQWrapper/IIPCTimeoutException.h
        ../ZeroMQWrapper/IPoller.h
        ../ZeroMQWrapper/IPollerFactory.h
        ../ZeroMQWrapper/IProxy.h
        ../ZeroMQWrapper/IReadWrite.h
        ../ZeroMQWrapper/IReadable.h
        ../ZeroMQWrapper/ISocketPublisher.h
        ../ZeroMQWrapper/ISocketReplier.h
        ../ZeroMQWrapper/ISocketRequester.h
        ../ZeroMQWrapper/ISocketSetup.h
        ../ZeroMQWrapper/ISocketSubscriber.h
        ../ZeroMQWrapper/IWritable.h
        )
target_include_directories(zeromqwrapperimplobject PUBLIC ${ZeroMQ_INCLUDE_DIR})
add_library(zeromqwrapperimpl SHARED $<TARGET_OBJECTS:zeromqwrapperimplobject>)
target_link_libraries(zeromqwrapperimpl PUBLIC ${ZeroMQ_LIBRARY} threads)

SET_TARGET_PROPERTIES(zeromqwrapperimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS zeromqwrapperimpl
        LIBRARY DESTINATION ${BASE_DIST_PREFIX}/files/base/lib64)
