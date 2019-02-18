/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Common/ZeroMQWrapper/IReadable.h"

#include <string>
#include <vector>
namespace Common
{
    namespace Reactor
    {
        /**
         *  Class to inform Reactor that it should stop by throwing the class as an exception. It is to be called from
         * the process. This is mainly intended, if a reactor is to be used as a server and to allow a command to quit
         * reactor.
         *
         *  @note It does not inherit from std::exception because it does not need to provide any error description.
         *  @note See ReactorImpl for how it is used. And TestListener for example of it being used to stop reactor.
         */
        class StopReactorRequestException
        {
        };

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
             * @note The CallbackListener can instruct the IReactor to stop by throwing a StopReactorRequest when the
             * process is called.
             */
            virtual void messageHandler(Common::ZeroMQWrapper::IReadable::data_t request) = 0;
        };

    } // namespace Reactor
} // namespace Common
