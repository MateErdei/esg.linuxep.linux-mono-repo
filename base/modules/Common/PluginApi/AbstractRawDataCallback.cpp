/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "AbstractRawDataCallback.h"
#include <Common/PluginApiImpl/Logger.h>

#include <Common/EventTypesImpl/EventConverter.h>
#include <Common/PluginApi/ApiException.h>

using namespace Common::EventTypes;

namespace Common
{
    namespace PluginApiImpl
    {
        void AbstractRawDataCallback::receiveData(const std::string& key, const std::string& data)
        {
            if(key == "Detector.Credentials")
            {
                EventConverter converter;
                Common::EventTypes::CredentialEvent event = *converter.createEventFromString<CredentialEvent>(data).get();
                processEvent(event);
            }
            else if(key == "Detector.PortScanning")
            {
                EventConverter converter;
                Common::EventTypes::PortScanningEvent event = *converter.createEventFromString<PortScanningEvent>(data).get();
                processEvent(event);
            }
            else
            {
                LOGERROR("Unknown event received, received event id = '" << key << "'");
            }
        }

        void AbstractRawDataCallback::processEvent(Common::EventTypes::CredentialEvent event)
        {

        }

        void AbstractRawDataCallback::processEvent(Common::EventTypes::PortScanningEvent event)
        {

        }
    }
}