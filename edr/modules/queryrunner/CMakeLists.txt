sophos_add_library(queryrunnerimpl SHARED
        IQueryRunner.h
        QueryRunnerImpl.h
        QueryRunnerImpl.cpp
        ParallelQueryProcessor.h
        ParallelQueryProcessor.cpp
        Logger.h
        Logger.cpp
        Telemetry.h
        Telemetry.cpp
        EXTRA_LIBS ${log4cpluslib}
        EXTRA_PROJECTS livequeryimpl
        EXTRA_INCLUDES ${CMAKE_SOURCE_DIR}/modules
        )
