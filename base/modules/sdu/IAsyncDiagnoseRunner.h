/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "ITaskQueue.h"
#include <memory>
namespace RemoteDiagnoseImpl
{
    class IAsyncDiagnoseRunner
    {
    public:
        virtual ~IAsyncDiagnoseRunner() = default;

        virtual void triggerDiagnose() = 0;

        virtual bool isRunning() = 0;

        virtual void triggerAbort() = 0;

        virtual bool hasTimedOut() = 0;
    };

    std::unique_ptr<IAsyncDiagnoseRunner> createDiagnoseRunner(
        std::shared_ptr<ITaskQueue>,
        std::string dirPath);

} // namespace UpdateScheduler