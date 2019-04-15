/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <map>
#include <mutex>
#include <string>
#include "TelemetryNode.h"
#include "TelemetryValue.h"

namespace Common::Telemetry
{

    class TelemetryDictionary : public TelemetryNode
    {
    public:
        TelemetryDictionary();
        ~TelemetryDictionary() override = default;
        TelemetryDictionary(const TelemetryDictionary& object) = default;
        TelemetryDictionary(TelemetryDictionary&& object) = default;
        TelemetryDictionary& operator=(const TelemetryDictionary& rhs) = default;
        TelemetryDictionary& operator=(TelemetryDictionary&& rhs) noexcept = default;

        void set(const std::string& key, const TelemetryNode& value);
        TelemetryNode& getNode(const std::string& key);
        bool keyExists(const std::string& key);

        void clear();

    private:
        std::map<std::string, TelemetryNode> m_nodes;
    };
}