/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IEventType.h"
#include "CredentialEvent.h"
#include "PortScanningEvent.h"

#include <string>
#include <memory>

namespace Common
{
    namespace EventTypes
    {
        class IEventConverter
        {
        public:
            virtual ~IEventConverter()= default;

            virtual const std::pair<std::string, std::string> eventToString(const IEventType* eventType) = 0;
            virtual CredentialEvent stringToCredentialEvent(const std::string& event) = 0;
            virtual PortScanningEvent stringToPortScanningEvent(const std::string& event) = 0;
        };

        extern std::unique_ptr<IEventConverter> constructEventConverter();
    }
}
