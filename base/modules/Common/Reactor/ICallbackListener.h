/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_ICALLBACKLISTENER_H
#define EVEREST_BASE_ICALLBACKLISTENER_H

#include <string>
#include <vector>

namespace Common
{
    namespace Reactor
    {
        enum class ProcessInstruction{ CONTINUE, QUIT};
        class ICallbackListener
        {
        public:
            virtual ~ICallbackListener() = default;
            virtual ProcessInstruction process(std::vector<std::string> ) = 0;
        };


    }
}

#endif //EVEREST_BASE_ICALLBACKLISTENER_H
