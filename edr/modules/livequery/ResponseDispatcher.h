/******************************************************************************************************

Copyright Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma  once

#include "QueryResponse.h"
#include "IResponseDispatcher.h"

namespace livequery{
    class ResponseDispatcher :  public IResponseDispatcher
    {
    public:
        void sendResponse(const std::string& correlationId, const QueryResponse& response) override;
    };
}
