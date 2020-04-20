/******************************************************************************************************

Copyright 2019-2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once
#include <livequery/ResponseStatus.h>
#include <functional>
#include <string>
#include <memory>

namespace queryrunner
{
    struct QueryRunnerStatus{
        std::string name;
        livequery::ErrorCode  errorCode;
        long queryDuration; 
        int rowCount; 
    };

    class IQueryRunner
    {
    public:
        virtual ~IQueryRunner() = default;
        virtual void triggerQuery(const std::string& correlationid, const std::string& query, std::function<void(std::string id)> notifyFinished) = 0;
        virtual std::string id() = 0; 
        virtual QueryRunnerStatus getResult() = 0; 
        virtual std::unique_ptr<IQueryRunner> clone() = 0;
    };
    std::unique_ptr<IQueryRunner> createQueryRunner(std::string osquerySocket); 
} // namespace queryrunner
