// Copyright 2021-2023 Sophos Limited. All rights reserved.
#pragma once

#include "IAsyncDiagnoseRunner.h"

#include "sdu/taskQueue/ITaskQueue.h"

#include <functional>
#include <memory>

namespace RemoteDiagnoseImpl::runnerModule
{
    class DiagnoseRunnerFactory
    {
        DiagnoseRunnerFactory();

    public:
        using FunctionType = std::function<std::unique_ptr<RemoteDiagnoseImpl::IAsyncDiagnoseRunner>(
            std::shared_ptr<RemoteDiagnoseImpl::ITaskQueue>,
            const std::string&)>;

        static DiagnoseRunnerFactory& instance();

        std::unique_ptr<RemoteDiagnoseImpl::IAsyncDiagnoseRunner> createDiagnoseRunner(
            std::shared_ptr<RemoteDiagnoseImpl::ITaskQueue> schedulerTaskQueue,
            const std::string& dirPath);

        // for tests only
        void replaceCreator(FunctionType creator);

        void restoreCreator();

    private:
        FunctionType m_creator;
    };
} // namespace RemoteDiagnoseImpl::runnerModule