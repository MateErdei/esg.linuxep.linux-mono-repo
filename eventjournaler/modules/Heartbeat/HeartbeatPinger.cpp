/******************************************************************************************************
Copyright 2021 Sophos Limited.  All rights reserved.
******************************************************************************************************/

#include "HeartbeatPinger.h"

namespace Heartbeat
{
    HeartbeatPinger::HeartbeatPinger(std::shared_ptr<bool> boolReference) :
        m_boolRef(std::move(boolReference))
    {
    }

    void HeartbeatPinger::ping()
    {
        *m_boolRef = true;
    }
} // namespace Heartbeat