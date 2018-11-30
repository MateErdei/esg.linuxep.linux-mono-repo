/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PortScanningDetectorCallback.h"

#include <Common/EventTypesImpl/EventConverter.h>

namespace Common
{
    namespace PluginApiImpl
    {
        Common::EventTypesImpl::PortScanningEvent PortScanningDetectorCallback::processIncomingPortScanningData()
        {
            Common::EventTypesImpl::EventConverter converter;
            return converter.createEventFromString<Common::EventTypesImpl::PortScanningEvent>(m_data);

        }


        void PortScanningDetectorCallback::receiveData(const std::string& key, const std::string& data)
        {
            m_eventTypeId = key;
            m_data = data;
        }
    }
}
