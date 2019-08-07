/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "SulDownloaderRunnerFactory.h"

#include "AsyncSulDownloaderRunner.h"

namespace UpdateSchedulerImpl
{
    namespace runnerModule
    {
        using namespace UpdateScheduler;

        /**Factory */

        SulDownloaderRunnerFactory::SulDownloaderRunnerFactory() { restoreCreator(); }

        SulDownloaderRunnerFactory& SulDownloaderRunnerFactory::instance()
        {
            static SulDownloaderRunnerFactory factory;
            return factory;
        }

        std::unique_ptr<IAsyncSulDownloaderRunner> SulDownloaderRunnerFactory::createSulDownloaderRunner(
            std::shared_ptr<SchedulerTaskQueue> schedulerTaskQueue,
            const std::string& dirPath)
        {
            return m_creator(schedulerTaskQueue, dirPath);
        }

        void SulDownloaderRunnerFactory::replaceCreator(FunctionType creator) { m_creator = creator; }

        void SulDownloaderRunnerFactory::restoreCreator()
        {
            m_creator = [](std::shared_ptr<SchedulerTaskQueue> schedulerTaskQueue, const std::string& dirPath) {
                return std::unique_ptr<IAsyncSulDownloaderRunner>(
                    new AsyncSulDownloaderRunner(schedulerTaskQueue, dirPath));
            };
        }
    } // namespace runnerModule
} // namespace UpdateSchedulerImpl
namespace UpdateScheduler
{
    std::unique_ptr<IAsyncSulDownloaderRunner> createSulDownloaderRunner(
        std::shared_ptr<SchedulerTaskQueue> schedulerTaskQueue,
        std::string dirPath)
    {
        using SulDownloaderRunnerFactory = UpdateSchedulerImpl::runnerModule::SulDownloaderRunnerFactory;
        return SulDownloaderRunnerFactory::instance().createSulDownloaderRunner(schedulerTaskQueue, dirPath);
    }
} // namespace UpdateScheduler