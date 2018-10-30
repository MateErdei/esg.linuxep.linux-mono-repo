
add_library(filesystemimplobject OBJECT
        ../FileSystem/IFileSystem.h
        FileSystemImpl.cpp
        FileSystemImpl.h
        ../FileSystem/IFilePermissions.h
        FilePermissionsImpl.h
        FilePermissionsImpl.cpp)

add_library(filesystemimpl SHARED $<TARGET_OBJECTS:filesystemimplobject>)

SET_TARGET_PROPERTIES( filesystemimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS filesystemimpl
        LIBRARY DESTINATION files/base/lib64)
