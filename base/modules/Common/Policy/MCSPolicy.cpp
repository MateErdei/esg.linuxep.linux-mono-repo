// Copyright 2023 Sophos Limited. All rights reserved.

#include "Logger.h"
#include "MCSPolicy.h"
#include "PolicyHelper.h"
#include "PolicyParseException.h"

#include "Common/OSUtilities/IIPUtils.h"

#include <chrono>

using namespace Common::Policy;

MCSPolicy::MCSPolicy(const std::string& xmlPolicy)
{
    Common::XmlUtilities::AttributesMap attributesMap{ {}, {} };
    try
    {
        attributesMap = Common::XmlUtilities::parseXml(xmlPolicy);
    }
    catch (const Common::XmlUtilities::XmlUtilitiesException& ex)
    {
        LOGERROR("Failed to parse policy: " << ex.what());
        std::throw_with_nested(Common::Policy::PolicyParseException(LOCATION, ex.what()));
    }

    extractMessageRelays(attributesMap);

}

void MCSPolicy::extractMessageRelays(const Common::XmlUtilities::AttributesMap &attributesMap)
{
    //message relays
    auto messageRelayEntities =
            attributesMap.entitiesThatContainPath("policy/configuration/messageRelays/messageRelay");

    if (messageRelayEntities.empty())
    {
        return;
    }
    std::vector<MessageRelay> messageRelays;
    std::string certificateFileContent;

    for (auto& messageRelay : messageRelayEntities)
    {
        auto attributes = attributesMap.lookup(messageRelay);
        messageRelays.emplace_back(attributes.value("priority"), attributes.value("address"), attributes.value("port"), attributes.value("id"));
    }
    messageRelay_ = sortMessageRelays(messageRelays);

}

std::vector<MessageRelay> MCSPolicy::getMessageRelays()
{
    return messageRelay_;
}

std::vector<std::string> MCSPolicy::getMessageRelaysHostNames()
{
// address:port
    std::vector<std::string> hostnames;
    for (auto& relay : messageRelay_)
    {
        std::string temp = relay.address + ":" + relay.port;
        hostnames.push_back(temp);
    }
    return hostnames;
}

std::vector<MessageRelay> MCSPolicy::sortMessageRelays(const std::vector<MessageRelay> &relays)
{
    // requirement: update caches candidates must be sorted by the following criteria:
    //  1. priority
    //  2. ip-proximity
    std::vector<std::string> cacheUrls;
    cacheUrls.reserve(relays.size());
    for (auto& candidate : relays)
    {
        cacheUrls.emplace_back(Common::OSUtilities::tryExtractServerFromHttpURL(candidate.address));
    }

    auto start = std::chrono::steady_clock::now();
    Common::OSUtilities::SortServersReport report = Common::OSUtilities::indexOfSortedURIsByIPProximity(cacheUrls);
    auto elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
    std::string logReport = reportToString(report);
    LOGSUPPORT("Sort update cache took " << elapsed << " ms. Entries: " << logReport);
    std::vector<int> sortedIndex = Common::OSUtilities::sortedIndexes(report);

    std::vector<MessageRelay> orderedCaches;
    orderedCaches.reserve(report.servers.size());
    for (auto& entry : report.servers)
    {
        orderedCaches.emplace_back(relays.at(entry.originalIndex));
    }

    std::stable_sort(
            orderedCaches.begin(),
            orderedCaches.end(),
            [](const MessageRelay& a, const MessageRelay& b) { return std::stoi(a.priority) < std::stoi(b.priority); });
    return orderedCaches;
}