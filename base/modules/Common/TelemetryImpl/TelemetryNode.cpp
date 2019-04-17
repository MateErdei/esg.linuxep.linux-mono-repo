/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TelemetryNode.h"


namespace Common::Telemetry
{
    TelemetryNode::TelemetryNode()
        :TelemetryNode("", NodeType::unknown)
    {
    }

    TelemetryNode::TelemetryNode(const NodeType type)
        :TelemetryNode("", type)
    {
    }

    TelemetryNode::TelemetryNode(const std::string keyName, const NodeType type)
        : m_key(keyName), m_nodeType(type)
    {
    }

    NodeType TelemetryNode::getType() const noexcept
    {
        return m_nodeType;
    }

    std::string TelemetryNode::getKey() const
    {
        return m_key;
    }
}
