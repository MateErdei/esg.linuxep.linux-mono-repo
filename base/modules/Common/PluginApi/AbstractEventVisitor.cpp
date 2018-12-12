/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "AbstractEventVisitor.h"
#include "Logger.h"

#include <Common/PluginApi/ApiException.h>
#include <Common/EventTypesImpl/EventConverter.h>

using namespace Common::EventTypes;

namespace Common
{
    namespace PluginApiImpl
    {

        void AbstractEventVisitor::processEvent(Common::EventTypes::CredentialEvent)
        {
        }

        void AbstractEventVisitor::processEvent(Common::EventTypes::PortScanningEvent)
        {
        }

        void AbstractEventVisitor::receiveData(const std::string& key, const std::string&)
        {
        }
    }
}