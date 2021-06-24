/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <string>

namespace SubscriberLib
{
    class Subscriber
    {
    public:
        void subscribeToEvents();
    private:
        std::string m_socketPath = "/opt/sophos-spl/plugins/eventjournaler/var/event.ipc";
    };
}

