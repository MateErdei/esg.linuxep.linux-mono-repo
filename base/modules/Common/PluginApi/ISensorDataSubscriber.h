/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_ISENSORDATASUBSCRIBER_H
#define EVEREST_BASE_ISENSORDATASUBSCRIBER_H

#include <memory>
#include "ISensorDataCallback.h"

namespace Common
{
    namespace PluginApi
    {
        /**
         * Holds the threads that listen to the ipc channel for data arriving for the subscriber.
         *
         * When ISensorDataSubscriber is created it is given an ISensorDataCallback (@see IPluginResourceManagement::createSensorDataSubscriber)
         * and it is configured with the socket that listen to the ipc channel.
         *
         * This allow the ISensorDataSubscriber to forward data incoming to the ipc channel to the
         * ISensorDataCallback::receiveData.
         *
         */
        class ISensorDataSubscriber
        {
        public:
            virtual  ~ISensorDataSubscriber() = default;

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
    }
}

#endif //EVEREST_BASE_ISENSORDATASUBSCRIBER_H
