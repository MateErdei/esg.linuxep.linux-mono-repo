/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "SulDownloader/suldownloaderdata/Logger.h"

log4cplus::Logger& getSulDownloaderDataLogger()
{
    static log4cplus::Logger STATIC_LOGGER = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("suldownloaderdata"));
    return STATIC_LOGGER;
}
