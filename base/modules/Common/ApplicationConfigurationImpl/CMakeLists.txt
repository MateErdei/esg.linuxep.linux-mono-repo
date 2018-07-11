add_library(applicationconfigurationimpl SHARED
        ../ApplicationConfiguration/IApplicationConfiguration.h
        ApplicationConfiguration.cpp
        ApplicationConfiguration.h ../ApplicationConfiguration/IApplicationPathManager.h ApplicationPathManager.cpp ApplicationPathManager.h ../PluginApi/ApiException.h ../Datatypes/SophosCppStandard.h)

target_include_directories(applicationconfigurationimpl PUBLIC ../ApplicationConfiguration ../../Common)
install(TARGETS applicationconfigurationimpl
        LIBRARY DESTINATION files/base/lib64)