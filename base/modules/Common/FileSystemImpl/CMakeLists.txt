
add_library(filesystemimplobject OBJECT
        ../FileSystem/IFileSystem.h
        FileSystemImpl.cpp
        FileSystemImpl.h)

add_library(filesystemimpl SHARED $<TARGET_OBJECTS:filesystemimplobject> )

install(TARGETS filesystemimpl
        LIBRARY DESTINATION files/base/lib64)
