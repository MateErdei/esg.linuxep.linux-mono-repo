add_library(threadsobject OBJECT
        LockableData.h
        NotifyPipe.cpp
        NotifyPipe.h
        AbstractThread.h
        AbstractThread.cpp)

add_library(threads SHARED $<TARGET_OBJECTS:threadsobject>)

SET_TARGET_PROPERTIES(threads
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS threads
        LIBRARY DESTINATION ${BASE_DIST_PREFIX}/files/base/lib64)
