
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

target_link_libraries(filesystemimplobject INTERFACE sslimpl utilityimpl cap)

add_library(filesystemimpl SHARED)

target_link_libraries(filesystemimpl PUBLIC filesystemimplobject)

SET_TARGET_PROPERTIES(filesystemimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS filesystemimpl
        LIBRARY DESTINATION ${BASE_DIST_PREFIX}/files/base/lib64)

