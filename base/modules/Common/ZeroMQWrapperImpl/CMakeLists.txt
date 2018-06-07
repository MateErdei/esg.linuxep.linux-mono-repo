add_library(zeromqwrapperimpl ContextImpl.cpp ContextImpl.h ContextHolder.cpp ContextHolder.h SocketHolder.cpp SocketHolder.h
        ../ZeroMQ_wrapper/IContext.h
        ../ZeroMQ_wrapper/IHasFD.h
        ../ZeroMQ_wrapper/IReadable.h
        ../ZeroMQ_wrapper/IReadWrite.h
        ../ZeroMQ_wrapper/ISocketPublisher.h
        ../ZeroMQ_wrapper/ISocketReplier.h
        ../ZeroMQ_wrapper/ISocketRequester.h
        ../ZeroMQ_wrapper/ISocketSetup.h
        ../ZeroMQ_wrapper/ISocketSubscriber.h
        ../ZeroMQ_wrapper/IWritable.h
        SocketRequesterImpl.cpp SocketRequesterImpl.h SocketImpl.cpp SocketImpl.h SocketReplierImpl.cpp SocketReplierImpl.h SocketUtil.cpp SocketUtil.h)
target_link_libraries(zeromqwrapperimpl PUBLIC ${ZeroMQ_LIBRARY} )

