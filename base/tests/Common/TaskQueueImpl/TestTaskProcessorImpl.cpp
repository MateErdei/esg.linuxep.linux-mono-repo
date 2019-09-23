/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/TaskQueueImpl/TaskProcessorImpl.h>
#include <Common/TaskQueueImpl/TaskQueueImpl.h>
#include <include/gtest/gtest.h>

TEST(TestTaskProcessorImpl, Construction) // NOLINT
{
    std::shared_ptr<Common::TaskQueue::ITaskQueue> queue(new Common::TaskQueueImpl::TaskQueueImpl());
    Common::TaskQueueImpl::TaskProcessorImpl processor(queue);
}

TEST(TestTaskProcessorImpl, StartAndStop) // NOLINT
{
    std::shared_ptr<Common::TaskQueue::ITaskQueue> queue(new Common::TaskQueueImpl::TaskQueueImpl());
    Common::TaskQueueImpl::TaskProcessorImpl processor(queue);
    processor.start();
    processor.stop();
}

TEST(TestTaskProcessorImpl, QueueNullTask) // NOLINT
{
    std::shared_ptr<Common::TaskQueue::ITaskQueue> queue(new Common::TaskQueueImpl::TaskQueueImpl());
    Common::TaskQueueImpl::TaskProcessorImpl processor(queue);
    processor.start();

    Common::TaskQueue::ITaskPtr task;
    queue->queueTask(std::move(task));

    processor.stop();
}

namespace
{
    class FakeTask : public virtual Common::TaskQueue::ITask
    {
    public:
        explicit FakeTask(bool& dest) : m_dest(dest) {}

        void run() { m_dest = true; }

        bool& m_dest;
    };
} // namespace

TEST(TestTaskProcessorImpl, CheckTaskExecuted) // NOLINT
{
    bool taskExecuted = false;
    Common::TaskQueue::ITaskPtr task(new FakeTask(taskExecuted));

    std::shared_ptr<Common::TaskQueue::ITaskQueue> queue(new Common::TaskQueueImpl::TaskQueueImpl());
    Common::TaskQueueImpl::TaskProcessorImpl processor(queue);
    processor.start();

    queue->queueTask(std::move(task));

    processor.stop();

    EXPECT_TRUE(taskExecuted);
}

TEST(TestTaskProcessorImpl, CheckTaskIsExecutedBeforeStopTaskIsExecuted) // NOLINT
{
    bool task1Executed = false;
    bool task2Executed = false;
    Common::TaskQueue::ITaskPtr task1(new FakeTask(task1Executed));
    Common::TaskQueue::ITaskPtr task2(new FakeTask(task2Executed));

    std::shared_ptr<Common::TaskQueue::ITaskQueue> queue(new Common::TaskQueueImpl::TaskQueueImpl());
    Common::TaskQueueImpl::TaskProcessorImpl processor(queue);

    queue->queueTask(std::move(task1));
    queue->queueTask(std::move(task2));

    processor.start(); // Will wait for thread to be running
    processor.stop();  // Will wait for all tasks to be completed

    EXPECT_TRUE(task1Executed);
    EXPECT_TRUE(task2Executed);
}
