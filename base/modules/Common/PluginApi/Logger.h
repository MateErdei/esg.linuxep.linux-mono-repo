/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>

log4cplus::Logger& getPluginAPIInterfaceLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getPluginAPIInterfaceLogger(), x) // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getPluginAPIInterfaceLogger(), x) // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_INFO(getPluginAPIInterfaceLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getPluginAPIInterfaceLogger(), x) // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getPluginAPIInterfaceLogger(), x) // NOLINT

