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
        enum class QueryResponseStatus{QueryFailedValidation, QueryInvalidJson, QueryResponseProduced, UnexpectedExceptionOnHandlingQuery }; 
        virtual ~IResponseDispatcher() = default;
        virtual void sendResponse(const std::string& correlationId, const QueryResponse& response) = 0;
        virtual std::unique_ptr<IResponseDispatcher> clone() = 0;
        /// Add method to allow for reporting what happened to the query since it was received till it is processed.
        virtual void feedbackResponseStatus(QueryResponseStatus ){
            // default behaviour is to ignore this information.
        };
        virtual std::string getTelemetry() = 0;
        virtual void setTelemetry(const std::string& json) = 0;
    };
} // namespace livequeryimpl
