//
// Created by pair on 07/06/18.
//

#ifndef EVEREST_LOGGER_H
#define EVEREST_LOGGER_H

#include <iostream>
namespace SulDownloader
{
    class Logger
    {

    };
}

#define LOGDEBUG(x) std::cout << __FILE__ << __LINE__ << x << '\n'
#define LOGINFO(x) std::cout << x << '\n'
#define LOGSUPPORT(x) std::cout << x << '\n'
#define LOGWARN(x) std::cerr << x << '\n'
#define LOGERROR(x) std::cerr << x << '\n'



#endif //EVEREST_LOGGER_H
