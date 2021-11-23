/******************************************************************************************************
Copyright 2021 Sophos Limited.  All rights reserved.
******************************************************************************************************/

#include <algorithm>
#include "Heartbeat.h"

namespace Heartbeat
{

    Heartbeat::Heartbeat() = default;

//    void Heartbeat::ping(const std::string& id)
//    {
//        *(m_registeredIds[id]) = true;
//    }

    void Heartbeat::deregisterId(const std::string& id)
    {
        m_registeredIds.erase(id);
    }

    std::vector<std::string> Heartbeat::getMissedHeartbeats()
    {
        std::vector<std::string> missedHeartbeats{};
        for (auto& kv: m_registeredIds)
        {
            if (!kv.second->isAlive())
            {
                missedHeartbeats.push_back(kv.first);
            }
        }
        return missedHeartbeats;

    }

    void Heartbeat::registerId(const std::string& id)
    {
        auto it = m_registeredIds.find(id);
        if (it == m_registeredIds.end())
        {
            m_registeredIds[id] = std::make_shared<HeartbeatPinger>();
        }
    }

    // TODO handle missing, perhaps return optional here.
    std::shared_ptr<HeartbeatPinger> Heartbeat::getPingHandleForId(const std::string &id)
    {
        registerId(id);
        return m_registeredIds[id];
    }

    std::vector<std::string> Heartbeat::getAllHeartbeatIds()
    {
        std::vector<std::string> heartbeatIds{};
        for (auto& kv: m_registeredIds)
        {
            heartbeatIds.push_back(kv.first);
        }
        return heartbeatIds;
    }

    uint Heartbeat::getNumDroppedEventsInLast24h()
    {
        uint numDroppedEvents = 0;
        for (auto& kv: m_registeredIds)
        {
            numDroppedEvents += kv.second->getNumDroppedEventsInLast24h();
        }
        return numDroppedEvents;
    }

    std::map<std::string, bool> Heartbeat::getMapOfIdsAgainstIsAlive()
    {
        std::map<std::string, bool> returnMap;
        for (auto& kv: m_registeredIds)
        {
            returnMap[kv.first] = kv.second->isAlive();
        }
        return returnMap;
    }

} // namespace Heartbeat