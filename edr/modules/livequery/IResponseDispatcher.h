/******************************************************************************************************

Copyright Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma  once

#include "QueryResponse.h"

namespace livequery{
    class IResponseDispatcher
    {
    public:
        virtual ~IResponseDispatcher()= default;
        virtual void sendResponse(const std::string& correlationId, const QueryResponse& response) = 0;
    };
}
