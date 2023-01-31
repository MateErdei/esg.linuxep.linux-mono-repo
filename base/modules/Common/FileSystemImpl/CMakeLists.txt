
add_library(filesystemimplobject OBJECT
        ../FileSystem/IFileSystem.h
        ../FileSystem/IFilePermissions.h
        ../FileSystem/IPidLockFileUtils.h
        ../FileSystem/IFileNotFoundException.h
        ../FileSystem/IFileSystemException.h
        ../FileSystem/IFileTooLargeException.h
        ../FileSystem/IPermissionDeniedException.h
        ../FileSystem/ITempDir.h
        FileSystemImpl.cpp
        FileSystemImpl.h
        FilePermissionsImpl.h
        FilePermissionsImpl.cpp
        PidLockFile.cpp
        PidLockFile.h
        TempDir.cpp
        TempDir.h)

add_library(filesystemimpl SHARED $<TARGET_OBJECTS:filesystemimplobject>)

target_link_libraries(filesystemimpl PUBLIC sslimpl utilityimpl)

SET_TARGET_PROPERTIES(filesystemimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS filesystemimpl
        LIBRARY DESTINATION files/base/lib64)

install(TARGETS filesystemimpl LIBRARY DESTINATION ${THININSTALLER_DEPS_DIR_LIB64})
