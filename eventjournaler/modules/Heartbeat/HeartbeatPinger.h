/******************************************************************************************************
Copyright 2021 Sophos Limited.  All rights reserved.
******************************************************************************************************/
#pragma once

#include <map>
#include <memory>

namespace Heartbeat
{
    class HeartbeatPinger
    {
    public:
        explicit HeartbeatPinger(std::shared_ptr<bool> boolReference);
        void ping();

    private:
        std::shared_ptr<bool> m_boolRef;
    };

} // namespace Heartbeat