/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "AbstractEventVisitor.h"

#include <Common/PluginApi/ApiException.h>
#include <Common/PluginApiImpl/Logger.h>
#include <Common/EventTypesImpl/EventConverter.h>

using namespace Common::EventTypes;

namespace Common
{
    namespace PluginApiImpl
    {

        void AbstractEventVisitor::processEvent(Common::EventTypes::CredentialEvent event)
        {

        }

        void AbstractEventVisitor::processEvent(Common::EventTypes::PortScanningEvent event)
        {

        }
    }
}