/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once
#include <Common/HttpSender/IHttpSender.h>
namespace Telemetry{
    class TelemetrySender : public Common::HttpSender::IHttpSender
    {
        int doHttpsRequest(const Common::HttpSender::RequestConfig& requestConfig) override;

    };
}