
add_library(filesystemimplobject OBJECT
        ../FileSystem/IFileSystem.h
        FileSystemImpl.cpp
        FileSystemImpl.h)

target_include_directories(filesystemimplobject PUBLIC ../FileSystem ../../Common)
add_library(filesystemimpl SHARED $<TARGET_OBJECTS:filesystemimplobject> )

install(TARGETS filesystemimpl
        LIBRARY DESTINATION files/base/lib64)
