// Copyright 2021-2023 Sophos Limited. All rights reserved.

#pragma once

#include "IEventQueue.h"

#include "Common/ZeroMQWrapper/IReadable.h"

#include <modules/JournalerCommon/Event.h>

#include <condition_variable>
#include <optional>
#include <queue>

namespace EventQueueLib
{
    class EventQueue : public IEventQueue
    {
    public:
        explicit EventQueue(int maxSize);

        bool push(JournalerCommon::Event event) override;
        std::optional<JournalerCommon::Event> pop(int timeoutInMilliseconds) override;

    protected:
        bool isQueueFull();
        bool isQueueEmpty();

        std::queue<JournalerCommon::Event> m_queue;

    private:
        uint m_maxQueueLength;
        std::mutex m_pushMutex;
        std::mutex m_popMutex;
        std::condition_variable m_cond;
    };
}
