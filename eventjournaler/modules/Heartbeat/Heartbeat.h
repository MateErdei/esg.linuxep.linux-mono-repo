/******************************************************************************************************
Copyright 2021 Sophos Limited.  All rights reserved.
******************************************************************************************************/
#pragma once

#include "IHeartbeat.h"
#include <map>

namespace Heartbeat
{
    class Heartbeat : public IHeartbeat
    {
    public:
        explicit Heartbeat(std::vector<std::string> ids);
//        void ping(const std::string& id) override;
        void registerId(const std::string& id) override;
        void deregisterId(const std::string& id) override;
        void resetAll() override;
        HeartbeatPinger getPingHandleForId(const std::string& id) override;
        std::vector<std::string> getMissedHeartbeats() override;
    private:
        std::map<std::string, std::shared_ptr<bool>> m_registeredIds;
    };
} // namespace Heartbeat