add_library(versionedcopylib STATIC
        VersionedCopy.cpp
        VersionedCopy.h)

target_link_libraries(versionedcopylib PUBLIC filesystemimpl)
