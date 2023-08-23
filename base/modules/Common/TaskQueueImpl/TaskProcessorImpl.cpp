// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "TaskProcessorImpl.h"

#include "Logger.h"

#include <utility>

Common::TaskQueueImpl::TaskProcessorImpl::TaskProcessorImpl(Common::TaskQueueImpl::ITaskQueueSharedPtr taskQueue) :
    m_thread(std::move(taskQueue))
{
}

Common::TaskQueueImpl::TaskProcessorImplThread::TaskProcessorImplThread(
    Common::TaskQueueImpl::ITaskQueueSharedPtr taskQueue) :
    m_taskQueue(std::move(taskQueue))
{
}

void Common::TaskQueueImpl::TaskProcessorImpl::start()
{
    m_thread.start();
}

namespace
{
    class StopTask : public virtual Common::TaskQueue::ITask
    {
    public:
        explicit StopTask(Common::Threads::AbstractThread& thread) : m_thread(thread) {}
        void run() override { m_thread.requestStop(); }

    private:
        Common::Threads::AbstractThread& m_thread;
    };
} // namespace

void Common::TaskQueueImpl::TaskProcessorImpl::stop()
{
    auto task = std::make_unique<StopTask>(m_thread);
    m_thread.m_taskQueue->queueTask(std::move(task));
    m_thread.join();
}

void Common::TaskQueueImpl::TaskProcessorImplThread::run()
{
    announceThreadStarted();
    while (!stopRequested())
    {
        Common::TaskQueue::ITaskPtr task = m_taskQueue->popTask();
        if (task)
        {
            try
            {
                task->run();
            }
            catch (std::exception& e)
            {
                LOGERROR("Failed to run task with error: " << e.what());
            }
        }
        else
        {
            LOGWARN("Failed to obtain task from queue");
        }
    }
}

Common::TaskQueueImpl::TaskProcessorImplThread::~TaskProcessorImplThread()
{
    // destructor must ensure that the thread is not running anymore or
    // seg fault may occur.
    requestStop();

    // Thread is running
    if (joinable())
    {
        // Add a stop task to wake up the queue
        auto task = std::make_unique<StopTask>(*this);
        m_taskQueue->queueTask(std::move(task));
    }

    join(); // Joins if joinable
}
