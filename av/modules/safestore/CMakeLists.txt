
add_library(safestoreimpl SHARED
        ISafeStoreWrapper.h
        SafeStoreWrapperImpl.h
        SafeStoreWrapperImpl.cpp
        Logger.cpp
        Logger.h
        Main.cpp
        Main.h
        SafeStoreRunner.h
        SafeStoreRunner.cpp)


message(${SAFESTORE_LIB})
target_link_libraries(safestoreimpl PUBLIC
        common
        ${SAFESTORE_LIB}
        )

target_include_directories(safestoreimpl PUBLIC ${SAFESTORE_INPUT})


install(TARGETS safestoreimpl
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)

SET_TARGET_PROPERTIES(safestoreimpl
        PROPERTIES
        BUILD_RPATH "$ORIGIN:${CMAKE_BINARY_DIR}/libs"
        INSTALL_RPATH "$ORIGIN:$ORIGIN/../../../base/lib64:/opt/sophos-spl/plugins/av/lib64:/opt/sophos-spl/base/lib64"
        )
