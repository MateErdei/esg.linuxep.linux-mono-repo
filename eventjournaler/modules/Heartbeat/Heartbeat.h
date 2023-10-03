// Copyright 2021 Sophos Limited. All rights reserved.
#pragma once

#include "IHeartbeat.h"
#include <map>

namespace Heartbeat
{
    class Heartbeat : public IHeartbeat
    {
    public:
        explicit Heartbeat();
        void deregisterId(const std::string& id) override;
        void registerIds(std::vector<std::string> ids) override;

        std::shared_ptr<HeartbeatPinger> getPingHandleForId(const std::string& id) override;
        std::vector<std::string> getAllHeartbeatIds() override;
        uint getNumDroppedEventsInLast24h() override;
        std::map<std::string, bool> getMapOfIdsAgainstIsAlive() override;

    private:
        void registerId(const std::string& id);
        std::map<std::string, std::shared_ptr<HeartbeatPinger>> m_registeredIds{};

    };
} // namespace Heartbeat