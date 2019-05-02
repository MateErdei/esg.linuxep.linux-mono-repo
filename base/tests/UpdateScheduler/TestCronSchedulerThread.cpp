/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <UpdateSchedulerImpl/cronModule/CronSchedulerThread.h>
#include <gmock/gmock-matchers.h>
#include <Common/UtilityImpl/TimeUtils.h>
#include <tests/Common/Helpers/FakeTimeUtils.h>
#include <Common/Logging/ConsoleLoggingSetup.h>

using namespace UpdateSchedulerImpl;
using namespace UpdateScheduler;
using milliseconds = std::chrono::milliseconds;
using minutes = std::chrono::minutes;
using time_point = std::chrono::steady_clock::time_point;
using CronSchedulerThread = cronModule::CronSchedulerThread;
namespace
{
    std::time_t t_20190501T13h{1556712000}; // Wednesday at 13:00
    std::time_t minute{60};
    std::time_t hour{ 60 * minute };
    std::time_t week{ hour * 24 * 7};

    time_point now() { return std::chrono::steady_clock::now(); }
    long elapsed_time_ms(time_point from, time_point to)
    {
        return std::chrono::duration_cast<milliseconds>(to - from).count();
    }

    long elapsed_time_ms(time_point from) { return elapsed_time_ms(from, now()); }


    // setup the utility that will replace calls to TimeUtils::getCurrentTime to the list of simulated times
    Common::UtilityImpl::ScopedReplaceITime setupSimulatedTimes( std::vector<time_t> simulated_times,
                                                                 std::shared_ptr<SchedulerTaskQueue> taskQueue )
    {
        std::unique_ptr<Common::UtilityImpl::ITime> mockTimer{
                new SequenceOfFakeTime( simulated_times, std::chrono::milliseconds(5), [taskQueue](){
                    taskQueue->pushStop();
                })
        };
        return Common::UtilityImpl::ScopedReplaceITime( std::move(mockTimer));
    }

    // helper methods to get all the entries in the queue up to the first stop
    std::vector<SchedulerTask::TaskType> getReceivedActions( SchedulerTaskQueue & queue)
    {
        std::vector<SchedulerTask::TaskType> receivedValues;
        while( true)
        {
            auto curr_value = queue.pop();
            if( curr_value.taskType != SchedulerTask::TaskType::Stop)
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


    // helper method for the tests which simulates CronSchedulerThread operation.
    std::vector<SchedulerTask::TaskType> receivedActionsForSimulatedTime( std::vector<time_t> simulated_entries,
                                                                          std::string & capturedLog,
                                                                          int offsetInMinutes , bool setUpdateOnStartUp)
    {
        Common::Logging::ConsoleLoggingSetup consoleLoggingSetup;
        ::testing::internal::CaptureStderr();
        std::shared_ptr<SchedulerTaskQueue> queue(new SchedulerTaskQueue());

        ScheduledUpdate scheduledUpdate;
        scheduledUpdate.setEnabled(true);

        Common::UtilityImpl::ScopedReplaceITime scopedReplaceITime{ setupSimulatedTimes(
                simulated_entries, queue
        ) };

        CronSchedulerThread schedulerThread(queue, std::chrono::milliseconds(10), std::chrono::milliseconds(10),
                                            offsetInMinutes, std::chrono::milliseconds(100));

        std::tm scheduledTime{};
        scheduledTime.tm_wday = 3; // Wednesday
        scheduledTime.tm_hour = 13; // 13:00
        scheduledUpdate.setScheduledTime(scheduledTime); // this is the way updatescheduler sets before passing to chron

        schedulerThread.setScheduledUpdate(scheduledUpdate);
        schedulerThread.setUpdateOnStartUp(setUpdateOnStartUp);


        schedulerThread.start();

        std::vector<SchedulerTask::TaskType> receivedValues = getReceivedActions(*queue);
        schedulerThread.requestStop();
        schedulerThread.join();
        (void) capturedLog;
        capturedLog = ::testing::internal::GetCapturedStderr();
        return receivedValues;
    }

}


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



TEST(TestCronSchedulerThread, simulateRunningFirstScheduledUpdate) // NOLINT
{
    std::vector<time_t> simulated_times{ t_20190501T13h - hour, t_20190501T13h, t_20190501T13h + hour};
    std::vector<SchedulerTask::TaskType> expectedValues{ SchedulerTask::TaskType::ScheduledUpdate };
    std::string capturedLog;
    auto receivedValues = receivedActionsForSimulatedTime( simulated_times, capturedLog, 0, false);
    ASSERT_EQ(receivedValues, expectedValues);
    EXPECT_THAT(capturedLog, ::testing::HasSubstr("Next Update scheduled to: 20190501 130000"));
}

TEST(TestCronSchedulerThread, simulateRunningFirstScheduledUpdateWithMinutes) // NOLINT
{
    std::vector<time_t> simulated_times;
    //simulate 30 minutes around the scheduled time, only one update schedule should be reported
    for(int i =-30; i<30; i++)
    {
        simulated_times.push_back(t_20190501T13h + i*minute  );
    }

    std::vector<SchedulerTask::TaskType> expectedValues{ SchedulerTask::TaskType::ScheduledUpdate };
    std::string capturedLog;
    auto receivedValues = receivedActionsForSimulatedTime( simulated_times, capturedLog, 5, false);
    ASSERT_EQ(receivedValues, expectedValues);
    EXPECT_THAT(capturedLog, ::testing::HasSubstr("Next Update scheduled to: 20190501 130000"));
}



TEST(TestCronSchedulerThread, simulateRunningTwoScheduledUpdate) // NOLINT
{
    std::vector<time_t> simulated_times{ t_20190501T13h - hour, t_20190501T13h, t_20190501T13h + hour,
            t_20190501T13h + week - hour, t_20190501T13h + week,
            t_20190501T13h + week + hour};
    std::vector<SchedulerTask::TaskType> expectedValues{ SchedulerTask::TaskType::ScheduledUpdate,
                                                         SchedulerTask::TaskType::ScheduledUpdate };
    std::string capturedLog;
    auto receivedValues = receivedActionsForSimulatedTime( simulated_times, capturedLog, 0, false);
    ASSERT_EQ(receivedValues, expectedValues);
    EXPECT_THAT(capturedLog, ::testing::HasSubstr("Next Update scheduled to: 20190501 130000"));
    EXPECT_THAT(capturedLog, ::testing::HasSubstr("Next Update scheduled to: 20190508 130000"));
}


TEST(TestCronSchedulerThread, simulateRunningThreeScheduledUpdate) // NOLINT
{
    std::vector<time_t> simulated_times{
           t_20190501T13h - hour, t_20190501T13h, t_20190501T13h + hour,
           t_20190501T13h + week - hour, t_20190501T13h + week, t_20190501T13h + week + hour,
           t_20190501T13h + 2*week - hour, t_20190501T13h + 2*week, t_20190501T13h + 2*week + hour
    };
    std::vector<SchedulerTask::TaskType> expectedValues{
        SchedulerTask::TaskType::ScheduledUpdate,
        SchedulerTask::TaskType::ScheduledUpdate,
        SchedulerTask::TaskType::ScheduledUpdate};
    std::string capturedLog;
    auto receivedValues = receivedActionsForSimulatedTime( simulated_times, capturedLog, 0, false);
    ASSERT_EQ(receivedValues, expectedValues);
    EXPECT_THAT(capturedLog, ::testing::HasSubstr("Next Update scheduled to: 20190501 130000"));
    EXPECT_THAT(capturedLog, ::testing::HasSubstr("Next Update scheduled to: 20190508 130000"));
    EXPECT_THAT(capturedLog, ::testing::HasSubstr("Next Update scheduled to: 20190515 130000"));

}


TEST(TestCronSchedulerThread, simulateRunningThreeScheduledUpdateWithRequiringUpdateOnStartUp) // NOLINT
{
    std::vector<time_t> simulated_times{
            t_20190501T13h - hour, t_20190501T13h, t_20190501T13h + hour,
            t_20190501T13h + week - hour, t_20190501T13h + week, t_20190501T13h + week + hour,
            t_20190501T13h + 2*week - hour, t_20190501T13h + 2*week, t_20190501T13h + 2*week + hour
    };
    std::vector<SchedulerTask::TaskType> expectedValues{
            SchedulerTask::TaskType::ScheduledUpdate, // an additional update will be executed because of the startup
            SchedulerTask::TaskType::ScheduledUpdate,
            SchedulerTask::TaskType::ScheduledUpdate,
            SchedulerTask::TaskType::ScheduledUpdate};
    std::string capturedLog;
    auto receivedValues = receivedActionsForSimulatedTime( simulated_times, capturedLog, 0, true);
    ASSERT_EQ(receivedValues, expectedValues);
    EXPECT_THAT(capturedLog, ::testing::HasSubstr("Next Update scheduled to: 20190501 130000"));
    EXPECT_THAT(capturedLog, ::testing::HasSubstr("Next Update scheduled to: 20190508 130000"));
    EXPECT_THAT(capturedLog, ::testing::HasSubstr("Next Update scheduled to: 20190515 130000"));

}


TEST(TestCronSchedulerThread, NoScheduledUpdateShouldBeTriggereBeforeScheduledTime) // NOLINT
{
    std::vector<time_t> simulated_times;
    //simulate 30 minutes before scheduled time
    for(int i =-30; i<-1; i++)
    {
        simulated_times.push_back(t_20190501T13h + i*minute  );
    }

    std::vector<SchedulerTask::TaskType> expectedValues{  };
    std::string capturedLog;
    auto receivedValues = receivedActionsForSimulatedTime( simulated_times, capturedLog, 5, false);
    ASSERT_EQ(receivedValues, expectedValues);
    EXPECT_THAT(capturedLog, ::testing::HasSubstr("Next Update scheduled to: 20190501 130000"));
}


TEST(TestCronSchedulerThread, cronSendUpdateTaskScheduledUpdateUpToOneMinuteBehind) // NOLINT
{
    std::vector<time_t> simulated_times;
    //simulate from 1 minute to 10
    for(int i =-1; i<10; i++)
    {
        simulated_times.push_back(t_20190501T13h + i*minute  );
    }

    std::vector<SchedulerTask::TaskType> expectedValues{ SchedulerTask::TaskType::ScheduledUpdate };
    std::string capturedLog;
    auto receivedValues = receivedActionsForSimulatedTime( simulated_times, capturedLog, 5, false);
    ASSERT_EQ(receivedValues, expectedValues);
    EXPECT_THAT(capturedLog, ::testing::HasSubstr("Next Update scheduled to: 20190501 130000"));
}

TEST(TestCronSchedulerThread, cronShouldNotTriggerUpdateBeforePassingTheOffsetInMinute) // NOLINT
{
    std::vector<time_t> simulated_times;
    //simulate from 1 minute to 6
    for(int i =-1; i<6; i++)
    {
        simulated_times.push_back(t_20190501T13h + i*minute  );
    }

    std::vector<SchedulerTask::TaskType> expectedValues{ SchedulerTask::TaskType::ScheduledUpdate };
    std::string capturedLog;
    int count_how_many_times_scheduled_update_was_triggered{0};
    // run simulation 10 times. Some times the offset will be lower than 6 minutes, sometimes it will be higher (50%  chance)
    for( int i =0; i<10; i++)
    {
        auto receivedValues = receivedActionsForSimulatedTime( simulated_times, capturedLog, 12, false);
        if( receivedValues.empty())
        {
            continue;
        }
        ASSERT_EQ(receivedValues.size(), 1); // there should always be either none or one.
        ASSERT_EQ(receivedValues, expectedValues); // if there is one, it must be the expectedValues
        count_how_many_times_scheduled_update_was_triggered++;
    }
    // statistically it is very unlikely that it has always triggered or never triggered.
    EXPECT_LT(count_how_many_times_scheduled_update_was_triggered, 10);
    EXPECT_GT(count_how_many_times_scheduled_update_was_triggered, 0);
}

