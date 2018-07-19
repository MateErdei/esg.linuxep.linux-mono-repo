add_library(taskqueueimpl SHARED
        TaskQueueImpl.cpp
        TaskQueueImpl.h
        ../TaskQueue/ITask.h
        ../TaskQueue/ITaskQueue.h)

target_include_directories(taskqueueimpl PUBLIC  )
target_link_libraries(taskqueueimpl PUBLIC threads)


install(TARGETS taskqueueimpl
        LIBRARY DESTINATION files/base/lib64)
