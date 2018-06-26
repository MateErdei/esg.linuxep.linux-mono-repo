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

            /**
             * Provides required implementation for how the data request should be processed.
             * Method will be invoked in the Reactor thread run method for all ICallbackListener implementations.
             * When Listener is added to the Reactor, and invoked.
             *
             * @param request, vector of strings which is the strings returned by IReadable::read
             * @return enum ProcessInstruction, either CONTINUE or QUIT
             */
            virtual ProcessInstruction process(std::vector<std::string> request) = 0;
        };


    }
}

#endif //EVEREST_BASE_ICALLBACKLISTENER_H
