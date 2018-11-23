/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "Logger.h"

log4cplus::Logger& getXmlUtilitiesLogger()
{
    static log4cplus::Logger STATIC_LOGGER = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("XmlUtilities"));
    return STATIC_LOGGER;
}
