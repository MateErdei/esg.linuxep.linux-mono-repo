// Copyright 2022 Sophos Limited. All rights reserved.

#pragma once

#include "scan_messages/ClientScanRequest.h"

namespace sophos_on_access_process::onaccessimpl
{
    class OnAccessScanRequest : public scan_messages::ClientScanRequest
    {
    public:
        using scan_messages::ClientScanRequest::ClientScanRequest;
    };
}
