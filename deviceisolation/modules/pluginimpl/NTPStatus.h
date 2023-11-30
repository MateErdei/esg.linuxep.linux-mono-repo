// Copyright 2023 Sophos Limited. All rights reserved.
#pragma once

#include <chrono>
#include <string>

namespace Plugin
{
    class NTPStatus
    {
    public:
        NTPStatus(std::string  revId, bool adminIsolationEnabled);
        [[nodiscard]] std::string xml() const;
        [[nodiscard]] std::string xmlWithoutTimestamp() const;

        static std::string generate_xml(const std::string& revId, bool adminIsolationEnabled, const std::string& timestamp);
        using clock_t = std::chrono::system_clock;
        using timepoint_t = clock_t::time_point;
        static std::string generate_xml(const std::string& revId, bool adminIsolationEnabled, const timepoint_t& timestamp);
    private:
        std::string revId_;
        bool adminIsolationEnabled_;
    };
}
