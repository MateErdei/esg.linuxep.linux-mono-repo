/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_LOGGER_H
#define EVEREST_LOGGER_H

#include <iostream>
namespace SulDownloader
{
    class Logger
    {

    };
}
//TODO: To incomporate the log facility after LINUXEP-5909 is completed.

#define LOGDEBUG(x) std::cout <<  x << '\n'
#define LOGINFO(x) std::cout << x << '\n'
#define LOGSUPPORT(x) std::cout << x << '\n'
#define LOGWARN(x) std::cerr << x << '\n'
#define LOGERROR(x) std::cerr << x << '\n'



#endif //EVEREST_LOGGER_H
