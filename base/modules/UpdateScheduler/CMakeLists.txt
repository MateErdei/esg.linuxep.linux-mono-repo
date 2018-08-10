add_library(updatescheduler SHARED UpdatePolicyTranslator.cpp UpdatePolicyTranslator.h SulDownloaderRunner.cpp SulDownloaderRunner.h SchedulerTaskQueue.cpp SchedulerTaskQueue.h SulDownloaderResultDirectoryListener.cpp SulDownloaderResultDirectoryListener.h)



target_link_libraries(updatescheduler PUBLIC suldownloaderimpl filesystemimpl utilityimpl applicationconfigurationimpl xmlutilities suldownloaderdata)

install(TARGETS updatescheduler
        LIBRARY DESTINATION files/base/lib64)


install(TARGETS updatescheduler
        LIBRARY DESTINATION files/base/lib64)