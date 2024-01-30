// Copyright 2018-2023 Sophos Limited. All rights reserved.
#pragma once

#include "Common/Logging/SophosLoggerMacros.h"

log4cplus::Logger& getXmlUtilitiesLogger();

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(getXmlUtilitiesLogger(), x)
#define LOGINFO(x) LOG4CPLUS_INFO(getXmlUtilitiesLogger(), x)
#define LOGSUPPORT(x) LOG4CPLUS_SUPPORT(getXmlUtilitiesLogger(), x)
#define LOGWARN(x) LOG4CPLUS_WARN(getXmlUtilitiesLogger(), x)
#define LOGERROR(x) LOG4CPLUS_ERROR(getXmlUtilitiesLogger(), x)
