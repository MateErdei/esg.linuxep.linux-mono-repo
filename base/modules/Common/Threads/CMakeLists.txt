add_library(threads SHARED
        NotifyPipe.cpp
        NotifyPipe.h
        AbstractThread.h
        AbstractThread.cpp)

install(TARGETS threads
        LIBRARY DESTINATION files/base/lib64)
