/******************************************************************************************************

Copyright 2019-2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "QueryResponse.h"
#include <memory>

namespace livequery
{
    class IResponseDispatcher
    {
    public:
        virtual ~IResponseDispatcher() = default;
        virtual void sendResponse(const std::string& correlationId, const QueryResponse& response) = 0;
        virtual std::unique_ptr<IResponseDispatcher> clone() = 0;
    };
} // namespace livequery
