/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TelemetryDictionary.h"



namespace Common::Telemetry
{

    TelemetryDictionary::TelemetryDictionary()
            : TelemetryNode(NodeType::dict)
    {
    }

    void TelemetryDictionary::set(const std::string& key, const std::shared_ptr<TelemetryNode>& value)
    {
        m_nodes[key] = value;
    }

    void TelemetryDictionary::clear()
    {
        m_nodes.clear();
    }

    std::shared_ptr<TelemetryNode> TelemetryDictionary::getNode(const std::string& key)
    {
        return m_nodes.at(key);
    }

    bool TelemetryDictionary::keyExists(const std::string& key)
    {
        return m_nodes.find(key) != m_nodes.end();
    }
}