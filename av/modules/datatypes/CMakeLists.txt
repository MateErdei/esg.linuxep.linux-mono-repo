
add_library(datatypes SHARED
        AutoFd.cpp
        AutoFd.h
        Print.h
        sophos_filesystem.h
        sophos_prctl.h
        Time.cpp
        Time.h
        IUuidGenerator.h
        UuidGeneratorImpl.cpp
        UuidGeneratorImpl.h
        AVException.h
)

target_link_libraries(datatypes  PUBLIC
        cap
        )
target_include_directories(datatypes PUBLIC ${pluginapiinclude})

install(TARGETS datatypes
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)

SET_TARGET_PROPERTIES(datatypes
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "$ORIGIN")
