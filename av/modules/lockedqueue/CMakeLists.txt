
add_library(lockedqueue SHARED
        LockedQueue.cpp
        LockedQueue.h
        )

target_include_directories(lockedqueue PUBLIC ${pluginapiinclude})
