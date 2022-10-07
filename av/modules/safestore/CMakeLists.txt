
add_library(safestoreimpl SHARED
        ISafeStoreWrapper.h
        IQuarantineManager.h
        Logger.cpp
        Logger.h
        Main.cpp
        Main.h
        QuarantineManagerImpl.h
        QuarantineManagerImpl.cpp
        SafeStoreObjectHandleHolderImpl.h
        SafeStoreObjectHandleHolderImpl.cpp
        SafeStoreSearchHandleHolderImpl.h
        SafeStoreSearchHandleHolderImpl.cpp
        SafeStoreWrapperImpl.h
        SafeStoreWrapperImpl.cpp
        StateMonitor.cpp
        StateMonitor.h
        )

message(${SAFESTORE_LIB})
target_link_libraries(safestoreimpl PUBLIC
        ${SAFESTORE_LIB}
        common
        ${log4cpluslib}
        )

target_include_directories(safestoreimpl PUBLIC ${SAFESTORE_INPUT})


install(TARGETS safestoreimpl
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)

SET_TARGET_PROPERTIES(safestoreimpl
        PROPERTIES
        BUILD_RPATH "$ORIGIN:${CMAKE_BINARY_DIR}/libs"
        INSTALL_RPATH "$ORIGIN:$ORIGIN/../../../base/lib64:/opt/sophos-spl/plugins/av/lib64:/opt/sophos-spl/base/lib64"
        )
