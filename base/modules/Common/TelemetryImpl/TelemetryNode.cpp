/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TelemetryNode.h"


namespace Common::Telemetry
{
    TelemetryNode::TelemetryNode(NodeType type)
            : m_nodeType(type)
    {
    }

    NodeType TelemetryNode::getType() const noexcept
    {
        return m_nodeType;
    }
}
