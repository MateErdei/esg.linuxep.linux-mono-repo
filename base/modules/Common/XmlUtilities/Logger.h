/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>

log4cplus::Logger& getXmlUtilitiesLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getXmlUtilitiesLogger(), x) // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(getXmlUtilitiesLogger(), x) // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_INFO(getXmlUtilitiesLogger(), x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(getXmlUtilitiesLogger(), x) // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(getXmlUtilitiesLogger(), x) // NOLINT
