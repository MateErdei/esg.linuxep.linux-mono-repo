add_library(systemcallwrapperobject OBJECT
        ISystemCallWrapper.h
        ISystemCallWrapperFactory.h
        SystemCallWrapper.h
        SystemCallWrapperFactory.cpp
        SystemCallWrapperFactory.h
        )


add_library(systemcallwrapper SHARED)

target_link_libraries(systemcallwrapper PUBLIC systemcallwrapperobject)

SET_TARGET_PROPERTIES(systemcallwrapper
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS systemcallwrapper
        LIBRARY DESTINATION ${BASE_DIST_PREFIX}/files/base/lib64)
