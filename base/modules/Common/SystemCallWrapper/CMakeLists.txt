add_library(systemcallwrapper OBJECT
        ISystemCallWrapper.h
        ISystemCallWrapperFactory.h
        SystemCallWrapper.h
        SystemCallWrapperFactory.cpp
        SystemCallWrapperFactory.h
        )

SET_TARGET_PROPERTIES(systemcallwrapper
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS systemcallwrapper
        LIBRARY DESTINATION ${BASE_DIST_PREFIX}/files/base/lib64)
