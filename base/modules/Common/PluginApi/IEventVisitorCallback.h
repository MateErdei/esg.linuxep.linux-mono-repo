/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IRawDataCallback.h"
#include <Common/EventTypes/ICredentialEvent.h>
#include <Common/EventTypes/IPortScanningEvent.h>

#include <string>

namespace Common
{
    namespace PluginApi
    {
        /**
         * Class that must be implemented when interested in subscribing to RawData.
         * @see IRawDataPublisher and ISubscriber.
         *
         * When data arrives in the ISubscriber that data will be forwarded to the
         * ::receiveData method of this callback.
         */
        class IEventVisitorCallback : public virtual IRawDataCallback
        {
        public :
            virtual ~IEventVisitorCallback() = default;

            virtual void processEvent(Common::EventTypes::ICredentialEventPtr event) = 0;
            virtual void processEvent(Common::EventTypes::IPortScanningEventPtr event) = 0;
        };
    }
}

