// Copyright 2019-2024 Sophos Limited. All rights reserved.
#pragma once
#include "ResponseStatus.h"

#include <functional>
#include <memory>
#include <string>

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
        /*Trigger the query and expect the query runner to call 'notifyFinished' when the query is finished. 
        NOTE: the call can not be in the body of 'triggerQuery' inline with the thread or deadlock will occur.
         Subsequent calls must wait until notifyFinished has been invoked by the previous call.
         */
        virtual void triggerQuery(const std::string& correlationid, const std::string& query, std::function<void(std::string id)> notifyFinished) = 0;
        /*Method to request the current query to abort 'as soon as possible' */
        virtual void requestAbort() = 0; 
        /* Return the same correlation id given in the triggerQuery*/
        virtual std::string id() = 0; 
        /* Return the result of the query executed, will be called by parallelQuery only after the queryrunner notifies finished. */
        virtual QueryRunnerStatus getResult() = 0; 
        /* Create a new instance of queryrunner to be used by the parallelQuery to run 'jobs' in parallel */
        virtual std::unique_ptr<IQueryRunner> clone() = 0;
    };
    std::unique_ptr<IQueryRunner> createQueryRunner(std::string osquerySocket,std::string executablePath);
} // namespace queryrunner
