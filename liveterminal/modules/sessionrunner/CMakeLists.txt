sophos_add_library(sessionrunnerimpl SHARED
        ISessionRunner.h
        SessionRunnerImpl.h
        SessionRunnerImpl.cpp
        ParallelSessionProcessor.h
        ParallelSessionProcessor.cpp
        Logger.h
        Logger.cpp
        EXTRA_PROJECTS common
        EXTRA_LIBS ${log4cpluslib} ${expatlib}
        EXTRA_INCLUDES ${CMAKE_SOURCE_DIR}/modules
)
