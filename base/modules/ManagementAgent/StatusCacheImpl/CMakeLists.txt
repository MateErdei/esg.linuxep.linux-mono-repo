add_library(statuscacheimpl SHARED
        StatusCache.h
        StatusCache.cpp
        ../StatusCache/IStatusCache.h
        )
target_include_directories(statuscacheimpl PUBLIC  ../../Common ${LOG4CPLUS_INCLUDE_DIR})

SET_TARGET_PROPERTIES( statuscacheimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS statuscacheimpl
        LIBRARY DESTINATION files/base/lib64)