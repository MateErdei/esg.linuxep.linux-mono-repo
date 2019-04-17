/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once
#include <json.hpp>

enum class NodeType
{
    unknown,
    value,
    dict
};

namespace Common::Telemetry
{
    class TelemetryNode
    {
    public:
        TelemetryNode();
        TelemetryNode(const NodeType type);
        TelemetryNode(const std::string keyName, const NodeType type);

        virtual ~TelemetryNode() = default;

        NodeType getType() const noexcept;

        std::string getKey() const;

    protected:
        std::string m_key;
        NodeType m_nodeType;
    };
}