/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/TaskQueueImpl/TaskQueueImpl.h>
#include <include/gtest/gtest.h>

namespace
{
    class FakeTask : public virtual Common::TaskQueue::ITask
    {
    public:
        bool m_taskRun;
        FakeTask() : m_taskRun(false) {}
        void run() override { m_taskRun = true; }
    };
} // namespace

TEST(TestQueueImpl, Construction) // NOLINT
{
    Common::TaskQueueImpl::TaskQueueImpl queue;
}

TEST(TestQueueImpl, addTask) // NOLINT
{
    Common::TaskQueueImpl::TaskQueueImpl queue;
    Common::TaskQueue::ITaskPtr task(new FakeTask());
    queue.queueTask(task);
    EXPECT_EQ(task, nullptr);
}

TEST(TestQueueImpl, removeTask) // NOLINT
{
    Common::TaskQueueImpl::TaskQueueImpl queue;
    Common::TaskQueue::ITaskPtr task(new FakeTask());
    Common::TaskQueue::ITask* taskBorrowedPointer = task.get();
    queue.queueTask(task);
    EXPECT_EQ(task, nullptr);
    task = queue.popTask();
    EXPECT_NE(task, nullptr);
    EXPECT_EQ(task.get(), taskBorrowedPointer);
}
