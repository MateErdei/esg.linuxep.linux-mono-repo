// Copyright 2021 Sophos Limited. All rights reserved.

#include <algorithm>
#include "Heartbeat.h"

namespace Heartbeat
{

    Heartbeat::Heartbeat() = default;

    void Heartbeat::deregisterId(const std::string& id)
    {
        m_registeredIds.erase(id);
    }

    void Heartbeat::registerId(const std::string& id)
    {
        auto it = m_registeredIds.find(id);
        if (it == m_registeredIds.end())
        {
            m_registeredIds[id] = std::make_shared<HeartbeatPinger>();
        }
    }

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
        for (const auto& [id, worker] : m_registeredIds)
        {
            returnMap[id] = worker->isAlive();
        }
        return returnMap;
    }

    void Heartbeat::registerIds(std::vector<std::string> ids)
    {
        for (auto id: ids)
        {
            registerId(id);
        }
    }

} // namespace Heartbeat