/******************************************************************************************************

Copyright 2019-2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "IResponseDispatcher.h"
#include "QueryResponse.h"
#include "Telemetry.h"

namespace livequery
{
    class ResponseDispatcher : public IResponseDispatcher
    {
    public:
        void sendResponse(const std::string& correlationId, const QueryResponse& response) override;
        std::string serializeToJson(const QueryResponse& response);

    private:
        Telemetry m_telemetryHandler;
    };
} // namespace livequery
