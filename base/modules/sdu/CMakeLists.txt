add_library(remotediagnoseimpl SHARED
        Scheduler.cpp
        Scheduler.h
        PluginCallback.cpp
        PluginCallback.h
        PluginAdapter.cpp
        PluginAdapter.h
        TaskQueue.cpp
        TaskQueue.h
        ITaskQueue.h
        Logger.cpp
        Logger.h)

include_directories(remotediagnoseimpl BEFORE ${LOG4CPLUS_INCLUDE_DIR} ../../../thirdparty/nlohmann-json)

target_link_libraries(remotediagnoseimpl PUBLIC
        pluginapiimpl
        processimpl
        utilityimpl
        osutilitiesimpl
        applicationconfigurationimpl
        xmlutilities
        )

SET_TARGET_PROPERTIES(remotediagnoseimpl
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

install(TARGETS remotediagnoseimpl
        LIBRARY DESTINATION files/base/lib64)

add_dependencies(remotediagnoseimpl copy_libs)
