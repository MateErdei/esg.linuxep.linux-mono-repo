/******************************************************************************************************

Copyright 2018-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TestWithMockTimerAbstract.h"

#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/UtilityImpl/TimeUtils.h>
#include <UpdateSchedulerImpl/cronModule/CronSchedulerThread.h>
#include <gmock/gmock-matchers.h>
#include <tests/Common/Helpers/FakeTimeUtils.h>


using namespace UpdateSchedulerImpl;
using namespace UpdateScheduler;
using milliseconds = std::chrono::milliseconds;
using minutes = std::chrono::minutes;
using time_point = std::chrono::steady_clock::time_point;
using CronSchedulerThread = cronModule::CronSchedulerThread;
namespace
{
    time_point now() { return std::chrono::steady_clock::now(); }
    long elapsed_time_ms(time_point from, time_point to)
    {
        return std::chrono::duration_cast<milliseconds>(to - from).count();
    }

    long elapsed_time_ms(time_point from) { return elapsed_time_ms(from, now()); }

} // namespace
class TestCronSchedulerThread : public TestWithMockTimerAbstract
{
public:
    // t_20190501T13h is defined as 2019-05-01 Wednesday at 13:00

    // setup the utility that will replace calls to TimeUtils::getCurrentTime to the list of simulated times
    Common::UtilityImpl::ScopedReplaceITime setupSimulatedTimes(
        std::vector<time_t> simulated_times,
        std::shared_ptr<SchedulerTaskQueue> taskQueue)
    {
        return TestWithMockTimerAbstract::setupSimulatedTimes(
            simulated_times, [taskQueue]() { taskQueue->pushStop(); });
    }

    // helper methods to get all the entries in the queue up to the first stop
    std::vector<SchedulerTask::TaskType> getReceivedActions(SchedulerTaskQueue& queue)
    {
        std::vector<SchedulerTask::TaskType> receivedValues;
        while (true)
        {
            auto curr_value = queue.pop();
            if (curr_value.taskType != SchedulerTask::TaskType::Stop)
            {
                receivedValues.push_back(curr_value.taskType);
            }
            else
            {
                break;
            }
        }
        return receivedValues;
    }

};

TEST_F(TestCronSchedulerThread, Constructor) // NOLINT
{
    std::shared_ptr<SchedulerTaskQueue> queue(new SchedulerTaskQueue());

    CronSchedulerThread schedulerThread(queue, milliseconds(10), milliseconds(20));
}

TEST_F(TestCronSchedulerThread, StartDestructor) // NOLINT
{
    std::shared_ptr<SchedulerTaskQueue> queue(new SchedulerTaskQueue());

    CronSchedulerThread schedulerThread(queue, minutes(10), minutes(20));
    schedulerThread.start();
}

TEST_F(TestCronSchedulerThread, CronSendQueueTaskOnRegularPeriod) // NOLINT
{
    std::shared_ptr<SchedulerTaskQueue> queue(new SchedulerTaskQueue());

    CronSchedulerThread schedulerThread(queue, milliseconds(100), milliseconds(200));
    time_point start_time = now();
    schedulerThread.start();

    ASSERT_EQ(queue->pop().taskType, SchedulerTask::TaskType::ScheduledUpdate);
    ASSERT_EQ(queue->pop().taskType, SchedulerTask::TaskType::ScheduledUpdate);
    // define expectation only for the lower bound, as the upper bound depends heavily on the machine burden.
    EXPECT_LE(300, elapsed_time_ms(start_time));
}

TEST_F(TestCronSchedulerThread, resetMethodRestablishRegularPeriod) // NOLINT
{
    std::shared_ptr<SchedulerTaskQueue> queue(new SchedulerTaskQueue());

    CronSchedulerThread schedulerThread(queue, milliseconds(10), milliseconds(100));
    time_point start_time = now();
    schedulerThread.start();

    ASSERT_EQ(queue->pop().taskType, SchedulerTask::TaskType::ScheduledUpdate);
    schedulerThread.setPeriodTime(milliseconds(200));
    schedulerThread.reset();

    ASSERT_EQ(queue->pop().taskType, SchedulerTask::TaskType::ScheduledUpdate);
    ASSERT_EQ(queue->pop().taskType, SchedulerTask::TaskType::ScheduledUpdate);
    ASSERT_EQ(queue->pop().taskType, SchedulerTask::TaskType::ScheduledUpdate);

    // define expectation only for the lower bound, as the upper bound depends heavily on the machine burden.
    EXPECT_LE(400, elapsed_time_ms(start_time));
}

TEST_F(TestCronSchedulerThread, requestStop) // NOLINT
{
    std::shared_ptr<SchedulerTaskQueue> queue(new SchedulerTaskQueue());

    CronSchedulerThread schedulerThread(queue, milliseconds(10), milliseconds(100));
    time_point start_time = now();
    schedulerThread.start();

    ASSERT_EQ(queue->pop().taskType, SchedulerTask::TaskType::ScheduledUpdate);
    ASSERT_EQ(queue->pop().taskType, SchedulerTask::TaskType::ScheduledUpdate);
    schedulerThread.requestStop();
    EXPECT_LE(110, elapsed_time_ms(start_time));
}

TEST_F(TestCronSchedulerThread, setTimeInMinutesButDoNotWait) // NOLINT
{
    std::shared_ptr<SchedulerTaskQueue> queue(new SchedulerTaskQueue());

    CronSchedulerThread schedulerThread(queue, std::chrono::minutes(10), std::chrono::minutes(60));
    time_point start_time = now();
    schedulerThread.start();

    std::thread stopInserter([&queue]() {
        std::this_thread::sleep_for(milliseconds(100));
        queue->pushStop();
    });

    ASSERT_EQ(queue->pop().taskType, SchedulerTask::TaskType::Stop);

    stopInserter.join();

    EXPECT_LE(100, elapsed_time_ms(start_time));

    EXPECT_GE(5000, elapsed_time_ms(start_time));
}
