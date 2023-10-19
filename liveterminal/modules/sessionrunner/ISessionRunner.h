// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once
#include <functional>
#include <memory>
#include <string>

namespace sessionrunner
{
    enum class ErrorCode : int
    {
        SUCCESS = 0,
        UNEXPECTEDERROR = 1
    };

    struct SessionRunnerStatus
    {
        ErrorCode errorCode;
    };

    class ISessionRunner
    {
    public:
        virtual ~ISessionRunner() = default;
        /*Trigger the session and expect the session runner to call 'notifyFinished' when the session is finished.
        NOTE: the call can not be in the body of 'triggerSession' inline with the thread or deadlock will occur.
         */
        virtual void triggerSession(const std::string& id, std::function<void(std::string id)> notifyFinished) = 0;
        /*Method to request the current session to abort 'as soon as possible' */
        virtual void requestAbort() = 0;
        /* Return the same id given in the triggerSession*/
        virtual std::string id() = 0;
        /* Create a new instance of sessionrunner to be used by the parallelQuery to run 'jobs' in parallel */
        virtual std::unique_ptr<ISessionRunner> clone() = 0;
    };
    std::unique_ptr<ISessionRunner> createSessionRunner(const std::string& executablePath);
} // namespace sessionrunner
