/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/EventTypesImpl/CredentialEvent.h>
#include <Common/EventTypesImpl/PortScanningEvent.h>

namespace Common
{
    namespace PluginApiImpl
    {

    class AbstractRawDataCallback
        {
        public:
            virtual ~AbstractRawDataCallback() = default;
            virtual Common::EventTypesImpl::CredentialEvent processIncomingCredentialData();
            virtual Common::EventTypesImpl::PortScanningEvent processIncomingPortScanningData();

        };
    }
}