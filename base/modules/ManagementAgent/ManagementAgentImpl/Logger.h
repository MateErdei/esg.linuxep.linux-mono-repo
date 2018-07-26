/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#ifndef EVEREST_BASE_LOGGER_H
#define EVEREST_BASE_LOGGER_H

#include <iostream>

#define LOGDEBUG(x) std::cout << x << std::endl // NOLINT
#define LOGINFO(x) std::cout << x << std::endl // NOLINT
#define LOGSUPPORT(x) std::cout << x << std::endl // NOLINT
#define LOGWARN(x) std::cerr << x << std::endl // NOLINT
#define LOGERROR(x) std::cerr << x << std::endl // NOLINT

#endif //EVEREST_BASE_LOGGER_H
