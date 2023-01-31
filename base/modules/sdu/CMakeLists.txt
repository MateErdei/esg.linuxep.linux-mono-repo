add_library(remotediagnoseimpl SHARED
        Main.cpp
        Main.h
        PluginCallback.cpp
        PluginCallback.h
        PluginAdapter.cpp
        PluginAdapter.h
        PluginUtils.cpp
        PluginUtils.h
        TaskQueue.cpp
        TaskQueue.h
        ITaskQueue.h
        Logger.cpp
        Logger.h
        IAsyncDiagnoseRunner.h
        runnerModule/AsyncDiagnoseRunner.cpp
        runnerModule/AsyncDiagnoseRunner.h
        runnerModule/DiagnoseResultDirectoryListener.h
        runnerModule/DiagnoseResultDirectoryListener.cpp
        runnerModule/DiagnoseRunner.cpp
        runnerModule/DiagnoseRunner.h
        runnerModule/DiagnoseRunnerFactory.cpp
        runnerModule/DiagnoseRunnerFactory.h
        )

include_directories(remotediagnoseimpl BEFORE ${LOG4CPLUS_INCLUDE_DIR} ${NLOHMANN_JSON_INCLUDE_DIR})

target_link_libraries(remotediagnoseimpl PUBLIC
        pluginapiimpl
        processimpl
        utilityimpl
        osutilitiesimpl
        applicationconfigurationimpl
        xmlutilities
        directorywatcherimpl
        watchdogservice
        )

SET_TARGET_PROPERTIES(remotediagnoseimpl
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

install(TARGETS remotediagnoseimpl
        LIBRARY DESTINATION base/files/base/lib64)

add_dependencies(remotediagnoseimpl copy_libs)
