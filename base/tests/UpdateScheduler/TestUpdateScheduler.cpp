/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <UpdateScheduler/UpdateSchedulerProcessor.h>
#include "MockAsyncDownloaderRunner.h"
#include "MockCronSchedulerThread.h"
#include <tests/Common/PluginApiImpl/MockApiBaseServices.h>
#include <UpdateScheduler/Logger.h>
#include <gmock/gmock-matchers.h>
#include <modules/UpdateScheduler/LoggingSetup.h>

using namespace UpdateScheduler;
using namespace Common::PluginApi;

class TestUpdateScheduler
        : public ::testing::Test
{

public:
    TestUpdateScheduler()
            : m_loggingSetup(std::unique_ptr<UpdateScheduler::LoggingSetup>(new UpdateScheduler::LoggingSetup(1)))
              , m_queue(std::make_shared<SchedulerTaskQueue>())
              , m_pluginCallback(std::make_shared<SchedulerPluginCallback>(m_queue))
    {
    }

    void TearDown() override
    {
    }

    std::unique_ptr<UpdateScheduler::LoggingSetup> m_loggingSetup;
    std::shared_ptr<SchedulerTaskQueue> m_queue;
    std::shared_ptr<SchedulerPluginCallback> m_pluginCallback;
};

TEST_F(TestUpdateScheduler, mainLoop) // NOLINT
{
    MockApiBaseServices* api = new MockApiBaseServices();
    MockAsyncDownloaderRunner* runner = new MockAsyncDownloaderRunner();
    MockCronSchedulerThread* cron = new MockCronSchedulerThread();

    UpdateScheduler::UpdateSchedulerProcessor
    updateScheduler(m_queue, std::unique_ptr<IBaseServiceApi>(api), m_pluginCallback,
                    std::unique_ptr<ICronSchedulerThread>(cron), std::unique_ptr<IAsyncSulDownloaderRunner>(runner));


}


