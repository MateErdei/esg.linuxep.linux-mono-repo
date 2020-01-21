
add_library(datatypes SHARED
        AutoFd.cpp
        AutoFd.h
        ../datatypes/Print.h )

install(TARGETS datatypes
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)
