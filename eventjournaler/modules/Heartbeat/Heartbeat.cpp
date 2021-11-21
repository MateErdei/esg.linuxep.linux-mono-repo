/******************************************************************************************************
Copyright 2021 Sophos Limited.  All rights reserved.
******************************************************************************************************/

#include <algorithm>
#include "Heartbeat.h"

namespace Heartbeat
{

    Heartbeat::Heartbeat(const std::vector<std::string> ids)
    {
        for (const std::string& id: ids)
        {
            registerId(id);
        }
    }

//    void Heartbeat::ping(const std::string& id)
//    {
//        *(m_registeredIds[id]) = true;
//    }

    void Heartbeat::deregisterId(const std::string& id)
    {
        m_registeredIds.erase(id);
    }

    void Heartbeat::resetAll()
    {
        for (const auto& kv: m_registeredIds)
        {
            *(kv.second) = false;
        }
    }

    std::vector<std::string> Heartbeat::getMissedHeartbeats()
    {
        std::vector<std::string> missedHeartbeats{};
        for (const auto& kv: m_registeredIds)
        {
            if (!*(kv.second))
            {
                missedHeartbeats.push_back(kv.first);
            }
        }
        return missedHeartbeats;

    }

    void Heartbeat::registerId(const std::string& id)
    {
        auto it = m_registeredIds.find(id);
        if (it != m_registeredIds.end())
        {
            *(m_registeredIds[id]) = false;
        }
        else
        {
            m_registeredIds[id] = std::make_shared<bool>(false);
        }

    }

    // TODO handle missing, perhaps return optional here.
    HeartbeatPinger Heartbeat::getPingHandleForId(const std::string &id)
    {
        return HeartbeatPinger(m_registeredIds[id]);
    }

} // namespace Heartbeat