/******************************************************************************************************

Copyright 2018-2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "Logger.h"

#include <Common/FileSystem/IFileSystem.h>

#include <modules/osqueryextensions/LoggerExtension.h>
#include <modules/pluginimpl/ApplicationPaths.h>
#include <iostream>

namespace {

    // XDR consts
    static const int DEFAULT_MAX_BATCH_SIZE_BYTES = 2000000;
    static const int DEFAULT_MAX_BATCH_TIME_SECONDS = 15;


    void registerAndStartLoggerPlugin(LoggerExtension& loggerExtension) {
        try {
            auto fs = Common::FileSystem::fileSystem();
            if (!fs->waitForFile(Plugin::osquerySocket(), 10000)) {
                std::cout<<"OSQuery socket does not exist after waiting 10 seconds. Restarting EDR";
                return;
            }
            loggerExtension.Start(Plugin::osquerySocket(),
                                    false,
                                    DEFAULT_MAX_BATCH_SIZE_BYTES,
                                    DEFAULT_MAX_BATCH_TIME_SECONDS);
        }
        catch (const std::exception &ex) {
            std::cout<<"Failed to start logger extension, loggerExtension.Start threw: " << ex.what();
        }
    }
}
int main()
{
}