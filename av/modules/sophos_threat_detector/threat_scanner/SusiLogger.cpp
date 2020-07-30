/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SusiLogger.h"

#include "Logger.h"

#include <string>

void threat_scanner::susiLogCallback(void* token, SusiLogLevel level, const char* message)
{
    static_cast<void>(token);
    std::string m(message);
    if (!m.empty())
    {
        switch (level)
        {
            case SUSI_LOG_LEVEL_DETAIL:
                LOGDEBUG(m);
                break;
            case SUSI_LOG_LEVEL_INFO:
                LOGINFO(m);
                break;
            case SUSI_LOG_LEVEL_WARNING:
                LOGWARN(m);
                break;
            case SUSI_LOG_LEVEL_ERROR:
                LOGERROR(m);
                break;
            default:
                LOGERROR(level << ": " << m);
                break;
        }
    }
}
