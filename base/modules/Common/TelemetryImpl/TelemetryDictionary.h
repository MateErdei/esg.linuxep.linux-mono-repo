/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <list>
#include <memory>
#include <string>
#include "TelemetryNode.h"
#include "TelemetryValue.h"

namespace Common::Telemetry
{

    class TelemetryDictionary : public TelemetryNode
    {
    public:
        TelemetryDictionary();
        TelemetryDictionary(const std::string& key);

        void set(const std::shared_ptr<TelemetryNode>& value);
        std::shared_ptr<TelemetryNode> getNode(const std::string& key) const;
        bool keyExists(const std::string& key) const;
        void clear();

        std::list<std::shared_ptr<TelemetryNode>> getDictionary() const;

        bool operator==(const TelemetryDictionary& rhs) const;
        bool operator!=(const TelemetryDictionary& rhs) const;

    private:
        std::list<std::shared_ptr<TelemetryNode>> m_nodes;
    };
}