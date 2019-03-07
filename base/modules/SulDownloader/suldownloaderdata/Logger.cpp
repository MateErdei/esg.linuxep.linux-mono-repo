/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "SulDownloader/suldownloaderdata/Logger.h"
#include <Common/Logging/LoggerConfig.h>

log4cplus::Logger& getSulDownloaderDataLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("suldownloaderdata");
    return STATIC_LOGGER;
}
