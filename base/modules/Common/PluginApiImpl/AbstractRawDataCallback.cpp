/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "AbstractRawDataCallback.h"

#include <Common/PluginApi/ApiException.h>

namespace Common
{
    namespace PluginApiImpl
    {
        Common::EventTypesImpl::CredentialEvent AbstractRawDataCallback::processIncomingCredentialData()
        {
            throw Common::PluginApi::ApiException("CredentialEvent ReceiveData not implemented");
        }

        Common::EventTypesImpl::PortScanningEvent AbstractRawDataCallback::processIncomingPortScanningData()
        {
            throw Common::PluginApi::ApiException("PortScanningEvent ReceiveData not implemented");
        }
    }
}