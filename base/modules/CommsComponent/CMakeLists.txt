add_library(commscomponentimpl SHARED 
        Comms.cpp 
        Logger.cpp 
        Logger.h 
        Comms.h
        AsyncMessager.h        
        AsyncMessager.cpp
        SplitProcesses.h
        MessageChannel.h
        MessageChannel.cpp
        Configurator.h
        ReactorAdapter.h
        ReactorAdapter.cpp
        OtherSideApi.h
        OtherSideApi.cpp
)


target_link_libraries(commscomponentimpl PUBLIC logging processimpl)

SET_TARGET_PROPERTIES( commscomponentimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

target_include_directories(commscomponentimpl PUBLIC ${BOOST_INCLUDE_DIR})

install(TARGETS commscomponentimpl
        LIBRARY DESTINATION files/base/lib64)

