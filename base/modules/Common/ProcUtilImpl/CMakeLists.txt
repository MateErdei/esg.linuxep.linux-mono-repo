add_library(procutilimplobject OBJECT
        ProcUtilities.h
        ProcUtilities.cpp
        Logger.h
        Logger.cpp
        )

target_include_directories(procutilimplobject PUBLIC ${LOG4CPLUS_INCLUDE_DIR})
add_library(procutilimpl SHARED $<TARGET_OBJECTS:procutilimplobject>)

SET_TARGET_PROPERTIES(procutilimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS procutilimpl
        LIBRARY DESTINATION files/base/lib64)