//
// Created by pair on 18/07/18.
//

#pragma once


#include <iostream>

//TODO: To incorporate the log facility after LINUXEP-5909 is completed.

#define LOGDEBUG(x) std::cout << x << std::endl // NOLINT
#define LOGINFO(x) std::cout << x << std::endl // NOLINT
#define LOGSUPPORT(x) std::cout << x << std::endl // NOLINT
#define LOGWARN(x) std::cerr << x << std::endl // NOLINT
#define LOGERROR(x) std::cerr << x << std::endl // NOLINT


