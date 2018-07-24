//
// Created by pair on 18/07/18.
//

#ifndef EVEREST_BASE_LOGGER_H
#define EVEREST_BASE_LOGGER_H

#include <iostream>

//TODO: To incorporate the log facility after LINUXEP-5909 is completed.

#define LOGDEBUG(x) std::cout << x << std::endl // NOLINT
#define LOGINFO(x) std::cout << x << std::endl // NOLINT
#define LOGSUPPORT(x) std::cout << x << std::endl // NOLINT
#define LOGWARN(x) std::cerr << x << std::endl // NOLINT
#define LOGERROR(x) std::cerr << x << std::endl // NOLINT

#endif //EVEREST_BASE_LOGGER_H
