/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once


enum class NodeType
{
    unknown,
    value,
    dict
};

class TelemetryNode
{
public:
    explicit TelemetryNode(NodeType type = NodeType::unknown);
    virtual ~TelemetryNode() = default;

    NodeType getType() const noexcept;

protected:
    NodeType m_type;
};