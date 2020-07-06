add_library(commscomponentimpl SHARED 
        Comms.cpp 
        Logger.cpp 
        Logger.h 
        Comms.h
        AsyncMessager.h
        AsyncMessager.cpp)

target_link_libraries(commscomponentimpl PUBLIC filesystemimpl processimpl threads utilityimpl applicationconfigurationimpl logging obfuscationimpl protobufutil)

target_include_directories(commscomponentimpl PUBLIC ${BOOST_INCLUDE_DIR})

install(TARGETS commscomponentimpl
        LIBRARY DESTINATION files/base/lib64)

