/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "AbstractRawDataCallback.h"
#include "Logger.h"

#include <Common/EventTypesImpl/EventConverter.h>
#include <Common/PluginApi/ApiException.h>

using namespace Common::EventTypesImpl;

namespace Common
{
    namespace PluginApiImpl
    {
        void AbstractRawDataCallback::receiveData(const std::string& key, const std::string& data)
        {
            if(key == "Detector.Credentials")
            {
                EventConverter converter;
                auto event = converter.createEventFromString<CredentialEvent>(data);
                processEvent(std::move(event));
            }
            else if(key == "Detector.PortScanning")
            {
                EventConverter converter;
                auto event = converter.createEventFromString<PortScanningEvent>(data);
                processEvent(std::move(event));
            }
            else
            {
                LOGERROR("Unknown event received, received event id = '" << key << "'");
            }
        }

        void AbstractRawDataCallback::processEvent(Common::EventTypes::ICredentialEventPtr event)
        {

        }

        void AbstractRawDataCallback::processEvent(Common::EventTypes::IPortScanningEventPtr event)
        {

        }
    }
}