add_library(commscomponentimpl SHARED Comms.cpp Logger.cpp Logger.h Comms.h)

target_link_libraries(commscomponentimpl PUBLIC filesystemimpl processimpl threads utilityimpl applicationconfigurationimpl logging obfuscationimpl protobufutil)

install(TARGETS commscomponentimpl
        LIBRARY DESTINATION files/base/lib64)

