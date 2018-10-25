
add_library(filepermissionsimplobject OBJECT
        ../FilePermissions/IFilePermissions.h
        FilePermissionsImpl.cpp
        FilePermissionsImpl.h)

add_library(filepermissionsimpl SHARED $<TARGET_OBJECTS:filepermissionsimplobject> )

SET_TARGET_PROPERTIES( filepermissionsimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS filepermissionsimpl
        LIBRARY DESTINATION files/base/lib64)
