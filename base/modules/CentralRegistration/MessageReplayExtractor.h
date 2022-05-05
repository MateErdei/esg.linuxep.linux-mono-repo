/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <cmcsrouter/MessageRelay.h>
#include <Common/UtilityImpl/StringUtils.h>

#include <string>
#include <vector>
#include <map>
#include <iostream>

std::vector<MCS::MessageRelay> extractMessageRelays(const std::string& messageRelaysAsString)
{
    std::vector<MCS::MessageRelay> messageRelays;
    std::vector<std::string> splitMessageRelays = Common::UtilityImpl::StringUtils::splitString(messageRelaysAsString, ";");
    for(auto& messageRelayAsString : splitMessageRelays)
    {
        std::vector<std::string> contents = Common::UtilityImpl::StringUtils::splitString(messageRelayAsString, ",");
        if(contents.size() != 3)
        {
            continue;
        }
        std::string priority = contents[1];
        std::string id = contents[2];
        std::vector<std::string> address = Common::UtilityImpl::StringUtils::splitString(contents[0], ":");
        if(address.size() != 2)
        {
            continue;
        }
        std::string hostname = address[0];
        std::string port = address[1];
        messageRelays.emplace_back(MCS::MessageRelay { priority, id, hostname, port });
    }
    return messageRelays;
}
