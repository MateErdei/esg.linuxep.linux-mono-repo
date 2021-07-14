/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <queue>
#include <condition_variable>
#include "Common/ZeroMQWrapper/IReadable.h"

namespace EventQueueLib
{
    class QueueEmptyException : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    class EventQueue
    {
    public:
        EventQueue(uint maxSize);
        bool push(Common::ZeroMQWrapper::data_t event);
        std::optional<Common::ZeroMQWrapper::data_t> pop(int timeoutInMilliseconds);

    protected:
        bool isQueueFull();
        bool isQueueEmpty();

        std::queue<Common::ZeroMQWrapper::data_t> m_queue;

    private:
        uint m_maxQueueLength;
        std::mutex m_pushMutex;
        std::mutex m_popMutex;
        std::condition_variable m_cond;
    };
}
