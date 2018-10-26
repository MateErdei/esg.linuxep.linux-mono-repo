
add_library(filesystemimplobject OBJECT
        ../FileSystem/IFileSystem.h
        FileSystemImpl.cpp
        FileSystemImpl.h)

add_library(filesystemimpl SHARED $<TARGET_OBJECTS:filesystemimplobject> $<TARGET_OBJECTS:filepermissionsimplobject>)

SET_TARGET_PROPERTIES( filesystemimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS filesystemimpl
        LIBRARY DESTINATION files/base/lib64)
