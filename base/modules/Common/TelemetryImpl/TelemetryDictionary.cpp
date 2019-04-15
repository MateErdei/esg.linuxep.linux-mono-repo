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

    void TelemetryDictionary::set(const std::string& key, const TelemetryNode& value)
    {
        m_nodes[key] = value;
    }

    void TelemetryDictionary::clear()
    {
        m_nodes.clear();
    }

    TelemetryNode& TelemetryDictionary::getNode(const std::string& key)
    {
        return m_nodes[key];
    }

    bool TelemetryDictionary::keyExists(const std::string& key)
    {
        return m_nodes.find(key) != m_nodes.end();
    }
}