/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <UpdateScheduler/CronSchedulerThread.h>
#include <gmock/gmock-matchers.h>

using namespace UpdateScheduler;
using milliseconds = std::chrono::milliseconds;
using minutes = std::chrono::minutes;
using time_point = std::chrono::steady_clock::time_point;

namespace
{
    time_point now()
    {
        return std::chrono::steady_clock::now();
    }
    int elapsed_time_ms( time_point from, time_point to)
    {
        return std::chrono::duration_cast<milliseconds>(to-from).count();
    }

    int elapsed_time_ms( time_point from)
    {
        return elapsed_time_ms(from, now());
    }
}

TEST(TestCronSchedulerThread, Constructor) // NOLINT
{
    std::shared_ptr<SchedulerTaskQueue> queue(new SchedulerTaskQueue());

    CronSchedulerThread schedulerThread(queue, milliseconds(10), milliseconds(20) );
}


TEST(TestCronSchedulerThread, StartDestructor) // NOLINT
{
    std::shared_ptr<SchedulerTaskQueue> queue(new SchedulerTaskQueue());

    CronSchedulerThread schedulerThread(queue, minutes(10), minutes(20) );
    schedulerThread.start();
}

TEST(TestCronSchedulerThread, CronSendQueueTaskOnRegularPeriod) // NOLINT
{
    std::shared_ptr<SchedulerTaskQueue> queue(new SchedulerTaskQueue());

    CronSchedulerThread schedulerThread(queue, milliseconds(100), milliseconds(200) );
    time_point start_time = now();
    schedulerThread.start();

    ASSERT_EQ(queue->pop().taskType, SchedulerTask::TaskType::ScheduledUpdate) ;
    ASSERT_EQ(queue->pop().taskType, SchedulerTask::TaskType::ScheduledUpdate) ;
    // define expectation only for the lower bound, as the upper bound depends heavily on the machine burden.
    EXPECT_LE(300, elapsed_time_ms(start_time));
}


TEST(TestCronSchedulerThread, resetMethodRestablishRegularPeriod) // NOLINT
{
    std::shared_ptr<SchedulerTaskQueue> queue(new SchedulerTaskQueue());

    CronSchedulerThread schedulerThread(queue, milliseconds(10), milliseconds(100) );
    time_point start_time = now();
    schedulerThread.start();

    ASSERT_EQ(queue->pop().taskType, SchedulerTask::TaskType::ScheduledUpdate) ;
    schedulerThread.setPeriodTime(milliseconds(200));
    schedulerThread.reset();

    ASSERT_EQ(queue->pop().taskType, SchedulerTask::TaskType::ScheduledUpdate) ;
    ASSERT_EQ(queue->pop().taskType, SchedulerTask::TaskType::ScheduledUpdate) ;
    ASSERT_EQ(queue->pop().taskType, SchedulerTask::TaskType::ScheduledUpdate) ;


    // define expectation only for the lower bound, as the upper bound depends heavily on the machine burden.
    EXPECT_LE(400, elapsed_time_ms(start_time));
}


TEST(TestCronSchedulerThread, requestStop) // NOLINT
{
    std::shared_ptr<SchedulerTaskQueue> queue(new SchedulerTaskQueue());

    CronSchedulerThread schedulerThread(queue, milliseconds(10), milliseconds(100) );
    time_point start_time = now();
    schedulerThread.start();

    ASSERT_EQ(queue->pop().taskType, SchedulerTask::TaskType::ScheduledUpdate) ;
    ASSERT_EQ(queue->pop().taskType, SchedulerTask::TaskType::ScheduledUpdate) ;
    schedulerThread.requestStop();
    EXPECT_LE(110, elapsed_time_ms(start_time));
}


TEST(TestCronSchedulerThread, setTimeInMinutesButDoNotWait) // NOLINT
{
    std::shared_ptr<SchedulerTaskQueue> queue(new SchedulerTaskQueue());

    CronSchedulerThread schedulerThread(queue, std::chrono::minutes(10), std::chrono::minutes(60) );
    time_point start_time = now();
    schedulerThread.start();

    std::thread stopInserter( [&queue](){std::this_thread::sleep_for(milliseconds(100)); queue->pushStop(); } );

    ASSERT_EQ(queue->pop().taskType, SchedulerTask::TaskType::Stop) ;

    stopInserter.join();

    EXPECT_LE(100, elapsed_time_ms(start_time));

    EXPECT_GE(5000, elapsed_time_ms(start_time));
}

