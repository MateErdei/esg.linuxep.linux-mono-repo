
add_library(safestoreimpl SHARED
        Logger.cpp
        Logger.h
        Main.cpp
        Main.h
        )

target_link_libraries(safestoreimpl PUBLIC
        common
        )

target_include_directories(safestoreimpl PUBLIC
        ${CMAKE_SOURCE_DIR}/modules)

install(TARGETS safestoreimpl
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)

SET_TARGET_PROPERTIES(safestoreimpl
        PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN:$ORIGIN/../../../base/lib64:/opt/sophos-spl/plugins/av/lib64:/opt/sophos-spl/base/lib64"
        )
