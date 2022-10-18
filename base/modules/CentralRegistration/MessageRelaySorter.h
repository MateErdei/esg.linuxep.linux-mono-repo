// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "cmcsrouter/MessageRelay.h"

#include <Common/OSUtilities/IIPUtils.h>

/**
 * Sorts message relays by:
 * 1. priority
 * 2. ip address proximity to local address(es). Distance between a pair of addresses is defined as the index of the
 * most significant bit that differs between them.
 *
 * Adapted from UpdatePolicyTranslator::sortUpdateCaches, and order_by_ip_address_distance in ip_selection.py
 */
std::vector<MCS::MessageRelay> sortMessageRelays(const std::vector<MCS::MessageRelay>& messageRelays)
{
    std::vector<std::string> urls;
    for (const auto& messageRelay : messageRelays)
    {
        urls.push_back(messageRelay.address);
    }

    Common::OSUtilities::SortServersReport report = Common::OSUtilities::indexOfSortedURIsByIPProximity(urls);

    std::vector<MCS::MessageRelay> orderedMessageRelays;
    for (const auto& entry : report.servers)
    {
        orderedMessageRelays.emplace_back(messageRelays.at(entry.originalIndex));
    }

    std::stable_sort(
        orderedMessageRelays.begin(),
        orderedMessageRelays.end(),
        [](const auto& a, const auto& b) { return a.priority < b.priority; });

    return orderedMessageRelays;
}