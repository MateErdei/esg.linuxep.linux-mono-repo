
add_library(datatypes SHARED
        AutoFd.cpp
        AutoFd.h
        Print.h )

install(TARGETS datatypes
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)
