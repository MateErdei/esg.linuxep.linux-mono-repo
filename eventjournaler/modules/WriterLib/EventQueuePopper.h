/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <optional>
#include "modules/EventQueueLib/IEventQueue.h"
#include "IEventQueuePopper.h"
#include "Common/ZeroMQWrapper/IReadable.h"

namespace WriterLib
{
    class EventQueuePopper : public IEventQueuePopper
    {
    public:
        EventQueuePopper(const std::shared_ptr<EventQueueLib::IEventQueue>& eventQueue);
        std::optional<Common::ZeroMQWrapper::data_t> getEvent(int timeoutInMilliseconds) override;

    private:
        std::shared_ptr<EventQueueLib::IEventQueue> m_eventQueue;
    };
}