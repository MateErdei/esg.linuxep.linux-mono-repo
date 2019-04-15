/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TelemetryNode.h"

TelemetryNode::TelemetryNode(NodeType type)
    : m_type(type)
{
}

NodeType TelemetryNode::getType() const noexcept
{
    return m_type;
}
