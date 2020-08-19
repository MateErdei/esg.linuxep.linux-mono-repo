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
                // SUSI is too verbose at info-level, downgrade to support-level
                LOGSUPPORT(m);
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
void threat_scanner::fallbackSusiLogCallback(void*, SusiLogLevel level, const char* message)
{

    std::string m(message);
    if (!m.empty())
    {
        switch (level)
        {
            case SUSI_LOG_LEVEL_DETAIL:
                std::cout << "DEBUG: " << m;
                break;
            case SUSI_LOG_LEVEL_INFO:
                std::cout << "INFO: " << m;
                break;
            case SUSI_LOG_LEVEL_WARNING:
                std::cout << "WARNING: " << m;
                break;
            case SUSI_LOG_LEVEL_ERROR:
                std::cerr << "ERROR: " << m;
                break;
            default:
                std::cerr << level << ": " << m;
                break;
        }
    }
}
