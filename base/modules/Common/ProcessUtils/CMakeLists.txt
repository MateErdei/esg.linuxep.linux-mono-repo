add_library(processutilsobject OBJECT
        ProcUtilities.h
        ProcUtilities.cpp
        Logger.h
        Logger.cpp
)
target_include_directories(processutilsobject PUBLIC ${LOG4CPLUS_INCLUDE_DIR} )
add_library(processutils SHARED $<TARGET_OBJECTS:processutilsobject>)

target_link_libraries(processutils
        filesystemimpl
        processimpl
        ${LOG4CPLUS_LIBRARY}
        )


#add_dependencies(procimpl copy_libs)

SET_TARGET_PROPERTIES( processutils
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS processutils
        LIBRARY DESTINATION files/base/lib64)