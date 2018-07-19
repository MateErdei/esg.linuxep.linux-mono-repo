/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include "TaskProcessorImpl.h"

Common::TaskQueueImpl::TaskProcessorImpl::TaskProcessorImpl(Common::TaskQueueImpl::ITaskQueueSharedPtr taskQueue)
        : m_thread(taskQueue)
{
}

Common::TaskQueueImpl::TaskProcessorImplThread::TaskProcessorImplThread(
        Common::TaskQueueImpl::ITaskQueueSharedPtr taskQueue)
        : m_taskQueue(taskQueue)
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
        explicit StopTask(Common::Threads::AbstractThread& thread)
                : m_thread(thread)
        {
        }
        void run()
        {
            m_thread.requestStop();
        }
    private:
        Common::Threads::AbstractThread& m_thread;
    };
}

void Common::TaskQueueImpl::TaskProcessorImpl::stop()
{
    Common::TaskQueue::ITaskPtr task(new StopTask(m_thread));
    m_thread.m_taskQueue->queueTask(task);
    m_thread.join();
}

void Common::TaskQueueImpl::TaskProcessorImplThread::run()
{
    announceThreadStarted();
    while ( !stopRequested())
    {
        Common::TaskQueue::ITaskPtr task = m_taskQueue->popTask();
        if (task != nullptr)
        {
            task->run();
        }
        else
        {
            // LOG?
        }
    }
}
