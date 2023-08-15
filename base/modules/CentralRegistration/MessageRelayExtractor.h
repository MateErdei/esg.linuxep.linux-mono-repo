// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Logger.h"

#include "Common/UtilityImpl/StringUtils.h"
#include "cmcsrouter/MessageRelay.h"

#include <iostream>
#include <map>
#include <string>
#include <vector>

std::vector<MCS::MessageRelay> extractMessageRelays(const std::string& messageRelaysAsString)
{
    std::vector<MCS::MessageRelay> messageRelays;
    std::vector<std::string> splitMessageRelays =
        Common::UtilityImpl::StringUtils::splitString(messageRelaysAsString, ";");
    for (const auto& messageRelayAsString : splitMessageRelays)
    {
        if (messageRelayAsString.empty())
        {
            continue;
        }

        std::vector<std::string> contents = Common::UtilityImpl::StringUtils::splitString(messageRelayAsString, ",");
        if (contents.size() != 3)
        {
            LOGWARN(
                "Ignoring message relay '" + messageRelayAsString +
                "' as it does not follow the <address:port,priority,id> format.");
            continue;
        }

        std::pair<int, std::string> priorityResult = Common::UtilityImpl::StringUtils::stringToInt(contents[1]);
        if (!priorityResult.second.empty())
        {
            LOGWARN("Ignoring message relay '" + messageRelayAsString + "' as it has non-integer priority.");
            continue;
        }
        int priority = priorityResult.first;

        std::string id = contents[2];
        std::vector<std::string> address = Common::UtilityImpl::StringUtils::splitString(contents[0], ":");
        if (address.size() != 2)
        {
            LOGWARN("Ignoring message relay '" + messageRelayAsString + "' as its address has wrong format.");
            continue;
        }
        std::string hostname = address[0];
        std::string port = address[1];
        messageRelays.emplace_back(MCS::MessageRelay{ priority, id, hostname, port });
    }
    return messageRelays;
}
