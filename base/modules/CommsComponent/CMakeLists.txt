include(proto/CMakeLists.txt)

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
        Configurator.cpp
        ReactorAdapter.h
        ReactorAdapter.cpp
        OtherSideApi.h
        OtherSideApi.cpp
        ${COMMS_MSG_PROTO_SRC}
        ${COMMS_MSG_PROTO_H}   
        CommsMsg.h 
        CommsMsg.cpp   
        NetworkSide.h 
        NetworkSide.cpp  
        )


target_link_libraries(commscomponentimpl PUBLIC 
        logging 
        processimpl 
        securityutils 
        utilityimpl 
        protobufutil 
        httpsenderimpl)

SET_TARGET_PROPERTIES( commscomponentimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

target_include_directories(commscomponentimpl PUBLIC 
        ..Common/SecurityUtils  
        ../Common/UtilityImpl 
        ${BOOST_INCLUDE_DIR}
        ${CURL_INCLUDE_DIR})

install(TARGETS commscomponentimpl
        LIBRARY DESTINATION files/base/lib64)

