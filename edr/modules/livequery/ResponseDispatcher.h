/******************************************************************************************************

Copyright 2019-2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "IResponseDispatcher.h"
#include "QueryResponse.h"
#include "modules/queryrunner/Telemetry.h"

namespace livequery
{
    class ResponseDispatcher : public IResponseDispatcher
    {
    public:
        void sendResponse(const std::string& correlationId, const QueryResponse& response) override;
        std::unique_ptr<IResponseDispatcher> clone() override ;
        std::string serializeToJson(const QueryResponse& response);
        std::string getTelemetry();
        void setTelemetry(const std::string& json);


    private:
        std::string m_telemetry = "";

    };
} // namespace livequery
