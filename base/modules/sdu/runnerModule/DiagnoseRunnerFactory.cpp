/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "DiagnoseRunnerFactory.h"

#include "AsyncDiagnoseRunner.h"

namespace RemoteDiagnoseImpl::runnerModule
{
    /**Factory */

    DiagnoseRunnerFactory::DiagnoseRunnerFactory() { restoreCreator(); }

    DiagnoseRunnerFactory& DiagnoseRunnerFactory::instance()
    {
        static DiagnoseRunnerFactory factory;
        return factory;
    }

    std::unique_ptr<IAsyncDiagnoseRunner> DiagnoseRunnerFactory::createDiagnoseRunner(
        std::shared_ptr<RemoteDiagnoseImpl::ITaskQueue> schedulerTaskQueue,
        const std::string& dirPath)
    {
        return m_creator(schedulerTaskQueue, dirPath);
    }

    void DiagnoseRunnerFactory::replaceCreator(FunctionType creator) { m_creator = creator; }

    void DiagnoseRunnerFactory::restoreCreator()
    {
        m_creator = [](std::shared_ptr<RemoteDiagnoseImpl::ITaskQueue> schedulerTaskQueue, const std::string& dirPath) {
            return std::unique_ptr<IAsyncDiagnoseRunner>(
                new AsyncDiagnoseRunner(schedulerTaskQueue, dirPath));
        };
    }
} // namespace RemoteDiagnoseImpl::runnerModule
namespace RemoteDiagnoseImpl
{
    std::unique_ptr<IAsyncDiagnoseRunner> createDiagnoseRunner(
        std::shared_ptr<ITaskQueue> schedulerTaskQueue,
        std::string dirPath)
    {
        using DiagnoseRunnerFactory = RemoteDiagnoseImpl::runnerModule::DiagnoseRunnerFactory;
        return DiagnoseRunnerFactory::instance().createDiagnoseRunner(schedulerTaskQueue, dirPath);
    }
} // namespace RemoteDiagnoseImpl