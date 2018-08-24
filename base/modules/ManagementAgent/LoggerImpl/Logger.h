/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>

extern log4cplus::Logger GL_MANAGEMENTAGENT_LOGGER;

#define LOGDEBUG(x) LOG4CPLUS_DEBUG(GL_MANAGEMENTAGENT_LOGGER, x) // NOLINT
#define LOGINFO(x) LOG4CPLUS_INFO(GL_MANAGEMENTAGENT_LOGGER, x) // NOLINT
#define LOGSUPPORT(x) LOG4CPLUS_INFO(GL_MANAGEMENTAGENT_LOGGER, x) // NOLINT
#define LOGWARN(x) LOG4CPLUS_WARN(GL_MANAGEMENTAGENT_LOGGER, x) // NOLINT
#define LOGERROR(x) LOG4CPLUS_ERROR(GL_MANAGEMENTAGENT_LOGGER, x) // NOLINT

