/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TelemetryDictionary.h"
#include <algorithm>

namespace Common::Telemetry
{

    TelemetryDictionary::TelemetryDictionary()
            : TelemetryDictionary("")
    {
    }

    TelemetryDictionary::TelemetryDictionary(const std::string& key)
            : TelemetryNode(key, NodeType::dict)
    {
    }

    void TelemetryDictionary::set(const std::shared_ptr<TelemetryNode>& value)
    {
        m_nodes.emplace_back(value);
    }

    void TelemetryDictionary::clear()
    {
        m_nodes.clear();
    }

    std::shared_ptr<TelemetryNode> TelemetryDictionary::getNode(const std::string& key) const
    {
        return *std::find_if(m_nodes.begin(), m_nodes.end(), [key](const auto& node){return node->getKey() == key;});
    }

    bool TelemetryDictionary::keyExists(const std::string& key) const
    {
        return std::find_if(m_nodes.begin(), m_nodes.end(), [key](const auto& node){return node->getKey() == key;}) != m_nodes.end();
    }

    std::list<std::shared_ptr<TelemetryNode>> TelemetryDictionary::getDictionary() const
    {
        return m_nodes;
    }

    bool TelemetryDictionary::operator==(const TelemetryDictionary& rhs) const
    {
        //return m_nodes == rhs.m_nodes;
        if (m_nodes.size() != rhs.m_nodes.size())
        {
            return false;
        }

        for(const auto& node: m_nodes)
        {
            if(!rhs.keyExists(node->getKey()))
            {
                return false;
            }

            switch(node->getType())
            {
                case NodeType::value:
                {
                    auto valueObj = std::static_pointer_cast<TelemetryValue>(node);
                    auto rhsValueObj = std::static_pointer_cast<TelemetryValue>(rhs.getNode(node->getKey()));

                    if(*valueObj != *rhsValueObj)
                    {
                        return false;
                    }
                    break;
                }

                case NodeType::dict:
                {
                    auto dictObj = std::static_pointer_cast<TelemetryDictionary>(node);
                    auto rhsDictObj = std::static_pointer_cast<TelemetryDictionary>(rhs.getNode(node->getKey()));

                    if(*dictObj != *rhsDictObj)
                    {
                        return false;
                    }
                    break;
                }

                default:
                {
                    throw std::invalid_argument("exception");
                }
            }
        }

        return true;
    }

    bool TelemetryDictionary::operator!=(const TelemetryDictionary& rhs) const
    {
        return !(rhs == *this);
    }
}