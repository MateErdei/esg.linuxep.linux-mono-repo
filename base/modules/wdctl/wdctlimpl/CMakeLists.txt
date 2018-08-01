add_library(wdctlimpl STATIC
        wdctl_main.cpp
        wdctl_main.h
        wdctl_bootstrap.cpp
        wdctl_bootstrap.h
        Arguments.cpp
        Arguments.h
        StringVector.h
        Logger.h
        )

target_link_libraries(wdctlimpl PUBLIC)
