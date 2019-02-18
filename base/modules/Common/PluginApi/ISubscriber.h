/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IEventVisitorCallback.h"

namespace Common
{
    namespace PluginApi
    {
        /**
         * Holds the threads that listen to the ipc channel for data arriving for the subscriber.
         *
         * When ISubscriber is created it is given an IEventVisitorCallback (@see
         * IPluginResourceManagement::createSubscriber) and it is configured with the socket that listen to the ipc
         * channel.
         *
         * This allow the ISubscriber to forward data incoming to the ipc channel to the
         * IEventVisitorCallback::processEvent.
         *
         */
        class ISubscriber
        {
        public:
            virtual ~ISubscriber() = default;

            /**
             * Start the thread that monitors the subscription ipc channel for incoming data.
             * It is not re-entrant. After starting/stop, it should not try to re-start.
             * Neither it should try to call start twice.
             */
            virtual void start() = 0;

            /**
             * Stop the thread the monitors the subscription of the ipc channel.
             * Stop may be called multiple times. But has no effect after the first call.
             */
            virtual void stop() = 0;
        };
    } // namespace PluginApi
} // namespace Common
