/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <UpdateSchedulerImpl/cronModule/CronSchedulerThread.h>
#include <gmock/gmock-matchers.h>

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

TEST(TestCronSchedulerThread, Constructor) // NOLINT
{
    std::shared_ptr<SchedulerTaskQueue> queue(new SchedulerTaskQueue());

    CronSchedulerThread schedulerThread(queue, milliseconds(10), milliseconds(20));
}

TEST(TestCronSchedulerThread, StartDestructor) // NOLINT
{
    std::shared_ptr<SchedulerTaskQueue> queue(new SchedulerTaskQueue());

    CronSchedulerThread schedulerThread(queue, minutes(10), minutes(20));
    schedulerThread.start();
}

TEST(TestCronSchedulerThread, CronSendQueueTaskOnRegularPeriod) // NOLINT
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

TEST(TestCronSchedulerThread, resetMethodRestablishRegularPeriod) // NOLINT
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

TEST(TestCronSchedulerThread, requestStop) // NOLINT
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

TEST(TestCronSchedulerThread, setTimeInMinutesButDoNotWait) // NOLINT
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

TEST(TestCronSchedulerThread, cronSendUpdateTaskScheduledUpdate) // NOLINT
{
    std::shared_ptr<SchedulerTaskQueue> queue(new SchedulerTaskQueue());

    CronSchedulerThread schedulerThread(queue, milliseconds(10), milliseconds(20), 0);

    ScheduledUpdate scheduledUpdate;
    scheduledUpdate.setEnabled(true);

    std::time_t timeT = std::time(nullptr);
    std::tm nowTm = *std::localtime(&timeT);
    scheduledUpdate.setScheduledTime(nowTm);

    schedulerThread.setScheduledUpdate(scheduledUpdate);
    schedulerThread.setUpdateOnStartUp(false);
    schedulerThread.start();

    std::thread stopInserter([&queue]() {
        std::this_thread::sleep_for(milliseconds(30));
        queue->pushStop();
    });

    ASSERT_EQ(queue->pop().taskType, SchedulerTask::TaskType::ScheduledUpdate);
    ASSERT_EQ(queue->pop().taskType, SchedulerTask::TaskType::Stop);

    stopInserter.join();
}

TEST(TestCronSchedulerThread, cronSendUpdateTaskScheduledUpdateUpToOneMinuteBehind) // NOLINT
{
    std::shared_ptr<SchedulerTaskQueue> queue(new SchedulerTaskQueue());

    CronSchedulerThread schedulerThread(queue, milliseconds(10), milliseconds(10), 0);

    ScheduledUpdate scheduledUpdate;
    scheduledUpdate.setEnabled(true);

    std::time_t nowTime = std::time(nullptr);
    std::time_t mod = nowTime % 60;
    std::time_t timeT = nowTime - mod;
    std::tm oneMinuteBehindTm = *std::localtime(&timeT);
    scheduledUpdate.setScheduledTime(oneMinuteBehindTm);

    schedulerThread.setScheduledUpdate(scheduledUpdate);
    schedulerThread.setUpdateOnStartUp(false);
    schedulerThread.start();

    std::thread stopInserter([&queue]() {
        std::this_thread::sleep_for(milliseconds(30));
        queue->pushStop();
    });

    ASSERT_EQ(queue->pop().taskType, SchedulerTask::TaskType::ScheduledUpdate);
    ASSERT_EQ(queue->pop().taskType, SchedulerTask::TaskType::Stop);

    stopInserter.join();
}

TEST(TestCronSchedulerThread, cronSendUpdateTaskScheduledUpdateUpToOneMinuteAhead) // NOLINT
{
    std::shared_ptr<SchedulerTaskQueue> queue(new SchedulerTaskQueue());

    CronSchedulerThread schedulerThread(queue, milliseconds(10), milliseconds(10), 0);

    ScheduledUpdate scheduledUpdate;
    scheduledUpdate.setEnabled(true);

    std::time_t nowTime = std::time(nullptr);
    std::time_t mod = nowTime % 60;
    std::time_t timeT = nowTime + (60 - mod);
    std::tm oneMinuteAheadTm = *std::localtime(&timeT);
    scheduledUpdate.setScheduledTime(oneMinuteAheadTm);

    schedulerThread.setScheduledUpdate(scheduledUpdate);
    schedulerThread.setUpdateOnStartUp(false);
    schedulerThread.start();

    std::thread stopInserter([&queue]() {
        std::this_thread::sleep_for(milliseconds(30));
        queue->pushStop();
    });

    ASSERT_EQ(queue->pop().taskType, SchedulerTask::TaskType::ScheduledUpdate);
    ASSERT_EQ(queue->pop().taskType, SchedulerTask::TaskType::Stop);

    stopInserter.join();
}

TEST(TestCronSchedulerThread, setScheduledTimeTwoMinutesButDoNotWait) // NOLINT
{
    std::shared_ptr<SchedulerTaskQueue> queue(new SchedulerTaskQueue());

    CronSchedulerThread schedulerThread(queue, std::chrono::milliseconds(10), std::chrono::milliseconds(10), 0);

    ScheduledUpdate scheduledUpdate;
    scheduledUpdate.setEnabled(true);

    std::time_t timeT = std::time(nullptr) + (2 * 60);
    std::tm twoMinutesAheadTm = *std::localtime(&timeT);
    scheduledUpdate.setScheduledTime(twoMinutesAheadTm);

    schedulerThread.setScheduledUpdate(scheduledUpdate);
    schedulerThread.setUpdateOnStartUp(false);
    schedulerThread.start();

    std::thread stopInserter([&queue]() {
        std::this_thread::sleep_for(milliseconds(30));
        queue->pushStop();
    });

    ASSERT_EQ(queue->pop().taskType, SchedulerTask::TaskType::Stop);

    stopInserter.join();
}

TEST(TestCronSchedulerThread, setScheduledTimeOneHourButDoNotWait) // NOLINT
{
    std::shared_ptr<SchedulerTaskQueue> queue(new SchedulerTaskQueue());

    CronSchedulerThread schedulerThread(queue, std::chrono::milliseconds(10), std::chrono::milliseconds(10), 0);

    ScheduledUpdate scheduledUpdate;
    scheduledUpdate.setEnabled(true);

    std::time_t timeT = std::time(nullptr) - (60 * 60);
    std::tm oneHourBehindTm = *std::localtime(&timeT);
    scheduledUpdate.setScheduledTime(oneHourBehindTm);

    schedulerThread.setScheduledUpdate(scheduledUpdate);
    schedulerThread.setUpdateOnStartUp(false);
    schedulerThread.start();

    std::thread stopInserter([&queue]() {
        std::this_thread::sleep_for(milliseconds(30));
        queue->pushStop();
    });
    ASSERT_EQ(queue->pop().taskType, SchedulerTask::TaskType::Stop);

    stopInserter.join();
}

TEST(TestCronSchedulerThread, setScheduledTimeOneDayButDoNotWait) // NOLINT
{
    std::shared_ptr<SchedulerTaskQueue> queue(new SchedulerTaskQueue());

    ScheduledUpdate scheduledUpdate;
    scheduledUpdate.setEnabled(true);

    CronSchedulerThread schedulerThread(queue, std::chrono::milliseconds(10), std::chrono::milliseconds(10), 0);
    std::time_t timeT = std::time(nullptr) + (24 * 60 * 60);
    std::tm oneDayAheadTm = *std::localtime(&timeT);
    scheduledUpdate.setScheduledTime(oneDayAheadTm);

    schedulerThread.setScheduledUpdate(scheduledUpdate);
    schedulerThread.setUpdateOnStartUp(false);
    schedulerThread.start();

    std::thread stopInserter([&queue]() {
        std::this_thread::sleep_for(milliseconds(30));
        queue->pushStop();
    });
    ASSERT_EQ(queue->pop().taskType, SchedulerTask::TaskType::Stop);

    stopInserter.join();
}

TEST(TestCronSchedulerThread, setScheduledTimeUpdateOnStart) // NOLINT
{
    std::shared_ptr<SchedulerTaskQueue> queue(new SchedulerTaskQueue());

    ScheduledUpdate scheduledUpdate;
    scheduledUpdate.setEnabled(true);

    CronSchedulerThread schedulerThread(queue, std::chrono::milliseconds(10), std::chrono::milliseconds(10), 0);
    std::time_t timeT = std::time(nullptr) + (24 * 60 * 60);
    std::tm oneDayAheadTm = *std::localtime(&timeT);
    scheduledUpdate.setScheduledTime(oneDayAheadTm);

    schedulerThread.setScheduledUpdate(scheduledUpdate);
    schedulerThread.setUpdateOnStartUp(true);
    schedulerThread.start();

    std::thread stopInserter([&queue]() {
        std::this_thread::sleep_for(milliseconds(30));
        queue->pushStop();
    });
    ASSERT_EQ(queue->pop().taskType, SchedulerTask::TaskType::ScheduledUpdate);
    ASSERT_EQ(queue->pop().taskType, SchedulerTask::TaskType::Stop);

    stopInserter.join();
}