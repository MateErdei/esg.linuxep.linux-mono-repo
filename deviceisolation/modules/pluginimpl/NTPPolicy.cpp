// Copyright 2023 Sophos Limited. All rights reserved.
#include "NTPPolicy.h"

#include "Logger.h"

#include "Common/XmlUtilities/AttributesMap.h"

using namespace Plugin;

namespace
{
    using Common::XmlUtilities::AttributesMap;
    std::vector<std::string> getContents(const AttributesMap& attributeMap, const std::string& entityPath)
    {
        auto entities = attributeMap.lookupMultiple(entityPath);
        std::vector<std::string> results;
        for (const auto& m : entities)
        {
            results.emplace_back(m.contents());
        }
        return results;
    }

    std::vector<IsolationExclusion> extractExclusions(const AttributesMap& attributeMap)
    {
        LOGDEBUG("Extracting exclusions");
        auto selfIsolation = attributeMap.lookupMultiple(("policy/configuration/selfIsolation"));
        if (selfIsolation.size() > 1)
        {
            throw NTPPolicyException(LOCATION, "Multiple selfIsolation in policy");
        }
        else if (selfIsolation.empty())
        {
            return {};
        }
        auto exclusions = attributeMap.entitiesThatContainPath("policy/configuration/selfIsolation/exclusions/exclusion", false);
        std::vector<IsolationExclusion> results;
        for (const auto& x : exclusions)
        {
            auto attr = attributeMap.lookup(x);
            if (attr.value("type") != "ip")
            {
                continue;
            }
            IsolationExclusion temp;
            auto directions = attributeMap.lookupMultiple(x + "/direction");
            if (directions.size() == 1)
            {
                auto direction = directions.at(0);
                if (direction.contents() == "in")
                {
                    temp.setDirection(IsolationExclusion::Direction::IN);
                }
                else if (direction.contents() == "out")
                {
                    temp.setDirection(IsolationExclusion::Direction::OUT);
                }
            }

            temp.setRemoteAddresses(getContents(attributeMap, x + "/remoteAddress"));
            temp.setLocalPorts(getContents(attributeMap, x + "/localPort"));
            temp.setRemotePorts(getContents(attributeMap, x + "/remotePort"));

            results.push_back(temp);
        }
        LOGDEBUG("Device Isolation using " << results.size() << " exclusions");
        return results;
    }
}

NTPPolicy::NTPPolicy(const std::string& xml)
{
    auto attributeMap = Common::XmlUtilities::parseXml(xml);
    auto compAttr = attributeMap.lookupMultiple("policy/csc:Comp");
    if (compAttr.size() != 1)
    {
        throw NTPPolicyException(LOCATION, "Incorrect number of csc:Comp in policy");
    }
    auto comp = compAttr.at(0);
    if (comp.value("policyType") != "24")
    {
        throw NTPPolicyException(LOCATION, "NTP Policy not type 24");
    }
    revId_ = comp.value("RevID", "unknown");

    exclusions_ = extractExclusions(attributeMap);
}

const std::string& NTPPolicy::revId() const
{
    return revId_;
}

const std::vector<IsolationExclusion>& NTPPolicy::exclusions() const
{
    return exclusions_;
}
