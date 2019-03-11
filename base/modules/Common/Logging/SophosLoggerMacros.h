/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>

#include <iostream>
namespace log4cplus
{
    const LogLevel SUPPORT_LOG_LEVEL     = DEBUG_LOG_LEVEL+1;
}

#define LOG4CPLUS_MACRO_SUPPORT_LOG_LEVEL(pred) \
    LOG4CPLUS_UNLIKELY (pred)

#define LOG4CPLUS_SUPPORT(logger, logEvent)                               \
    LOG4CPLUS_MACRO_BODY (logger, logEvent, SUPPORT_LOG_LEVEL)
#define LOG4CPLUS_SUPPORT_STR(logger, logEvent)                           \
    LOG4CPLUS_MACRO_STR_BODY (logger, logEvent, SUPPORT_LOG_LEVEL)
#define LOG4CPLUS_SUPPORT_FMT(logger, ...)                                \
    LOG4CPLUS_MACRO_FMT_BODY (logger, SUPPORT_LOG_LEVEL, __VA_ARGS__)

