// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "UpdateSettings.h"

#include "Common/XmlUtilities/AttributesMap.h"

#include <string>

namespace Common::Policy
{
    struct MessageRelay
    {
        std::string priority;
        std::string address;
        std::string port;
        std::string endpointId;

        MessageRelay(const std::string& priority, const std::string& address, const std::string& port,
                     const std::string& endpointID ) : priority(priority), address(address), port(port),
                     endpointId(endpointID)
                     {

                     }
    };
    class MCSPolicy
    {
    public:
        explicit MCSPolicy(const std::string& xmlPolicy);

        std::vector<MessageRelay> getMessageRelays();

        std::vector<std::string> getMessageRelaysHostNames();

        std::vector<MessageRelay> sortMessageRelays(const std::vector<MessageRelay>& relays);


    private:
        void extractMessageRelays(const Common::XmlUtilities::AttributesMap& attributesMap);

        std::vector<MessageRelay> messageRelay_;
    };
}