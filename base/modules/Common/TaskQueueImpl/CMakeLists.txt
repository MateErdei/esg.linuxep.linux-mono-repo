add_library(taskqueueimpl SHARED
        TaskQueueImpl.cpp
        TaskQueueImpl.h
        TaskProcessorImpl.cpp
        TaskProcessorImpl.h
        ../TaskQueue/ITask.h
        ../TaskQueue/ITaskQueue.h
        )

target_include_directories(taskqueueimpl PUBLIC  )
target_link_libraries(taskqueueimpl PUBLIC threads)

SET_TARGET_PROPERTIES( taskqueueimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS taskqueueimpl
        LIBRARY DESTINATION files/base/lib64)
