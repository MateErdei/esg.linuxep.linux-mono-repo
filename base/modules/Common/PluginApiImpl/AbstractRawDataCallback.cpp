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
                CredentialEvent event = converter.createEventFromString<CredentialEvent>(data);
                processEvent(event);
            }
            else if(key == "Detector.PortScanning")
            {
                EventConverter converter;
                PortScanningEvent event = converter.createEventFromString<PortScanningEvent>(data);
                processEvent(event);
            }
            else
            {
                LOGERROR("Unknown event received, received event id = '" << key << "'");
            }
        }

        void AbstractRawDataCallback::processEvent(Common::EventTypesImpl::CredentialEvent& event)
        {

            throw Common::PluginApi::ApiException("CredentialEvent ReceiveData not implemented");
        }

        void AbstractRawDataCallback::processEvent(PortScanningEvent& event)
        {
            throw Common::PluginApi::ApiException("PortScanningEvent ReceiveData not implemented");
        }
    }
}