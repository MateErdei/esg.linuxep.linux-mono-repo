add_library(zeromqwrapperimpl ContextImpl.cpp ContextImpl.h ContextHolder.cpp ContextHolder.h SocketHolder.cpp SocketHolder.h
        ../ZeroMQWrapper/IContext.h
        ../ZeroMQWrapper/IHasFD.h
        ../ZeroMQWrapper/IReadable.h
        ../ZeroMQWrapper/IReadWrite.h
        ../ZeroMQWrapper/ISocketPublisher.h
        ../ZeroMQWrapper/ISocketReplier.h
        ../ZeroMQWrapper/ISocketRequester.h
        ../ZeroMQWrapper/ISocketSetup.h
        ../ZeroMQWrapper/ISocketSubscriber.h
        ../ZeroMQWrapper/IWritable.h
        SocketRequesterImpl.cpp SocketRequesterImpl.h
        SocketImpl.cpp SocketImpl.h
        SocketReplierImpl.cpp SocketReplierImpl.h
        SocketUtil.cpp SocketUtil.h
        SocketPublisherImpl.cpp SocketPublisherImpl.h
        SocketSubscriberImpl.cpp SocketSubscriberImpl.h ../ZeroMQWrapper/IContextPtr.h ../ZeroMQWrapper/IIPCException.h ZeroMQWrapperException.cpp ZeroMQWrapperException.h ../ZeroMQWrapper/IIPCTimeoutException.h ZeroMQTimeoutException.cpp ZeroMQTimeoutException.h ../Exceptions/IException.h ../Exceptions/Print.h ../ZeroMQWrapper/IPoller.h PollerImpl.cpp PollerImpl.h)
target_link_libraries(zeromqwrapperimpl PUBLIC ${ZeroMQ_LIBRARY} )

