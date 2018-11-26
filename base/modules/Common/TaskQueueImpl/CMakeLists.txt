add_library(taskqueueimpl SHARED
        TaskQueueImpl.cpp
        TaskQueueImpl.h
        TaskProcessorImpl.cpp
        TaskProcessorImpl.h
        ../TaskQueue/ITask.h
        ../TaskQueue/ITaskQueue.h
        Logger.h
        Logger.cpp)

target_include_directories(taskqueueimpl PUBLIC  ${LOG4CPLUS_INCLUDE_DIR} )
target_link_libraries(taskqueueimpl PUBLIC threads
        ${LOG4CPLUS_LIBRARY})

SET_TARGET_PROPERTIES( taskqueueimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS taskqueueimpl
        LIBRARY DESTINATION files/base/lib64)
