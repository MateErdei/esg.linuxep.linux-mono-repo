/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "IEventQueue.h"
#include <queue>
#include <condition_variable>
#include "Common/ZeroMQWrapper/IReadable.h"

namespace EventQueueLib
{
    class EventQueue : public IEventQueue
    {
    public:
        EventQueue(int maxSize);
        bool push(Common::ZeroMQWrapper::data_t event) override;
        std::optional<Common::ZeroMQWrapper::data_t> pop(int timeoutInMilliseconds) override;

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
