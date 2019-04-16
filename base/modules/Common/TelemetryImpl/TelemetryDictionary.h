/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <map>
#include <mutex>
#include <string>
#include <memory>
#include "TelemetryNode.h"
#include "TelemetryValue.h"

namespace Common::Telemetry
{

    class TelemetryDictionary : public TelemetryNode
    {
    public:
        TelemetryDictionary();

        void set(const std::string& key, const std::shared_ptr<TelemetryNode>& value);
        std::shared_ptr<TelemetryNode> getNode(const std::string& key);
        bool keyExists(const std::string& key);

        void clear();

    private:
        std::map<std::string, std::shared_ptr<TelemetryNode>> m_nodes;
    };
}