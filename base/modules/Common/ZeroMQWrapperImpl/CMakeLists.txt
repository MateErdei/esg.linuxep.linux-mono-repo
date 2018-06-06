add_library(zeromqwrapperimpl ContextImpl.cpp ContextImpl.h ContextHolder.cpp ContextHolder.h SocketHolder.cpp SocketHolder.h)
target_link_libraries(zeromqwrapperimpl PUBLIC ${ZeroMQ_LIBRARY} )

