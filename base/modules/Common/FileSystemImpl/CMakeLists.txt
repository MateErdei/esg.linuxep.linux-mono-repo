
add_library(filesystemimpl SHARED
        ../FileSystem/IFileSystem.h
        FileSystemImpl.cpp
        FileSystemImpl.h)

target_include_directories(filesystemimpl PUBLIC ../FileSystem ../../Common)


install(TARGETS filesystemimpl
        LIBRARY DESTINATION files/base/lib64)
