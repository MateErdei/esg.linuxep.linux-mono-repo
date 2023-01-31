add_library(applicationconfigurationimplobject OBJECT
        ../ApplicationConfiguration/IApplicationConfiguration.h
        ApplicationConfiguration.cpp
        ApplicationConfiguration.h
        ../ApplicationConfiguration/IApplicationPathManager.h
        ApplicationPathManager.cpp
        ApplicationPathManager.h
        ../PluginApi/ApiException.h
        )

target_include_directories(applicationconfigurationimplobject PUBLIC ../ApplicationConfiguration ../../Common)
add_library(applicationconfigurationimpl SHARED $<TARGET_OBJECTS:applicationconfigurationimplobject>)

install(TARGETS applicationconfigurationimpl
        LIBRARY DESTINATION base/files/base/lib64)

install(TARGETS applicationconfigurationimpl LIBRARY DESTINATION ${THININSTALLER_DEPS_DIR_LIB64})