// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include <nlohmann/json.hpp>

#include <string>

namespace scan_messages
{
    struct Threat
    {
        std::string type;
        std::string name;
        std::string sha256;

        [[nodiscard]] bool operator==(const Threat& other) const noexcept
        {
            return std::tie(type, name, sha256) == std::tie(other.type, other.name, other.sha256);
        }
    };

    inline void to_json(nlohmann::json& j, const Threat& threat)
    {
        j = nlohmann::json{ { "type", threat.type }, { "name", threat.name }, { "sha256", threat.sha256 } };
    }

    inline void from_json(const nlohmann::json& j, Threat& threat)
    {
        j.at("type").get_to(threat.type);
        j.at("name").get_to(threat.name);
        j.at("sha256").get_to(threat.sha256);
    }
} // namespace scan_messages
