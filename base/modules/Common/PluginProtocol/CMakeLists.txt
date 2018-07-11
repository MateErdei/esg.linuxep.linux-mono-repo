add_library(pluginprotocol SHARED
        DataMessage.h
        DataMessage.cpp
        Logger.h)
install(TARGETS pluginprotocol
        LIBRARY DESTINATION files/base/lib64)