/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef WATCHDOG_WATCHDOGIMPL_LOGGER_H
#define WATCHDOG_WATCHDOGIMPL_LOGGER_H

#include <iostream>

//TODO: To incorporate the log facility after LINUXEP-5909 is completed.

#define LOGDEBUG(x) std::cout << x << std::endl // NOLINT
#define LOGINFO(x) std::cout << x << std::endl // NOLINT
#define LOGSUPPORT(x) std::cout << x << std::endl // NOLINT
#define LOGWARN(x) std::cerr << x << std::endl // NOLINT
#define LOGERROR(x) std::cerr << x << std::endl // NOLINT

#endif //WATCHDOG_WATCHDOGIMPL_LOGGER_H
