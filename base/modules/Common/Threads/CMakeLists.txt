add_library(threadsobject OBJECT
        NotifyPipe.cpp
        NotifyPipe.h
        AbstractThread.h
        AbstractThread.cpp)

add_library(threads SHARED $<TARGET_OBJECTS:threadsobject>)

SET_TARGET_PROPERTIES(threads
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS threads
        LIBRARY DESTINATION files/base/lib64)
