/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include <iostream>
namespace SulDownloader
{
    class Logger
    {

    };
}
//TODO LINUXEP-5909: To incomporate the log facility.

#define LOGDEBUG(x) std::cout <<  x << '\n'
#define LOGINFO(x) std::cout << x << '\n'
#define LOGSUPPORT(x) std::cout << x << '\n'
#define LOGWARN(x) std::cerr << x << '\n'
#define LOGERROR(x) std::cerr << x << '\n'




