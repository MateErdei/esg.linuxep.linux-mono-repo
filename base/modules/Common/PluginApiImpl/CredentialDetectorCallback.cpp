/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "CredentialDetectorCallback.h"
#include <Common/EventTypesImpl/EventConverter.h>

namespace Common
{
    namespace PluginApiImpl
    {
        Common::EventTypesImpl::CredentialEvent CredentialDetectorCallback::processIncomingCredentialData()
        {
            Common::EventTypesImpl::EventConverter converter;
            return converter.createEventFromString<Common::EventTypesImpl::CredentialEvent>(m_data);
        }

        void CredentialDetectorCallback::receiveData(const std::string& key, const std::string& data)
        {
            m_eventId = key;
            m_data = data;
        }
    }
}