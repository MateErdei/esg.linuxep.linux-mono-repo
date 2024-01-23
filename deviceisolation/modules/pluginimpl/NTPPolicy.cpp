// Copyright 2023 Sophos Limited. All rights reserved.
#include "NTPPolicy.h"

#include "Logger.h"

#include "Common/XmlUtilities/AttributesMap.h"
#include "Common/UtilityImpl/StringUtils.h"

#include <nlohmann/json.hpp>

#include <cassert>

using namespace Plugin;

namespace
{
    using Common::XmlUtilities::AttributesMap;
    std::vector<std::string> getContents(const AttributesMap& attributeMap, const std::string& entityPath, const std::function<bool(const std::string&)>& verifier = [](const std::string&){return true;} )
    {
        auto entities = attributeMap.lookupMultiple(entityPath);
        std::vector<std::string> results;
        for (const auto& m : entities)
        {
            if (verifier(m.contents()))
            {
                results.emplace_back(m.contents());
            }
            else
            {
                throw Common::Exceptions::IException(LOCATION, "Invalid value \"" + m.contents() + "\" for " + entityPath);
            }
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
                else
                {
                    LOGERROR("Invalid direction entry: " << direction.contents() << ", expect in or out");
                    continue;
                }
            }
            else if (directions.size() > 1)
            {
                LOGERROR("Invalid number of direction parameters: " << directions.size());
                continue;
            }

            // Extract and validate any remote addresses
            try
            {
                auto isValidAddress = [](const std::string &s){
                    return Common::UtilityImpl::StringUtils::isValidIpAddress(s, AF_INET)
                        || Common::UtilityImpl::StringUtils::isValidIpAddress(s, AF_INET6);
                };
                std::vector<std::string> remoteAddresses = getContents(attributeMap, x + "/remoteAddress", isValidAddress);

                std::vector<std::pair<std::string, std::string>> remoteAddressesAndIpTypes;
                for (const auto& remoteAddress : remoteAddresses)
                {
                    if (Common::UtilityImpl::StringUtils::isValidIpAddress(remoteAddress, AF_INET))
                    {
                        remoteAddressesAndIpTypes.emplace_back(remoteAddress, Plugin::IsolationExclusion::IPv4);
                    }
                    else
                    {
                        remoteAddressesAndIpTypes.emplace_back(remoteAddress, Plugin::IsolationExclusion::IPv6);
                    }
                }
                temp.setRemoteAddressesAndIpTypes(remoteAddressesAndIpTypes);
            }
            catch (std::runtime_error& exception)
            {
                LOGWARN("Invalid exclusion remote address: " << exception.what());
                continue;
            }

            // Extract and validate any ports
            auto isNumber = [](const std::string& s){return Common::UtilityImpl::StringUtils::isPositiveInteger(s);};
            try
            {
                temp.setLocalPorts(getContents(attributeMap, x + "/localPort", isNumber));
            }
            catch(const std::runtime_error& exception)
            {
                LOGWARN("Invalid exclusion local port: " << exception.what());
                continue;
            }

            try
            {
                temp.setRemotePorts(getContents(attributeMap, x + "/remotePort", isNumber));
            }
            catch(const std::runtime_error& exception)
            {
                LOGWARN("Invalid exclusion remote port: " << exception.what());
                continue;
            }

            results.push_back(temp);
        }
        LOGDEBUG("Device Isolation using " << results.size() << " exclusions");
        return results;
    }
}

NTPPolicy::NTPPolicy(const std::string& policy)
{
    auto attributeMap = openPolicy(policy);

    if (attributeMap.has_value())
    {
        auto compAttr = attributeMap.value().lookupMultiple("policy/csc:Comp");
        if (compAttr.size() != 1) {
            throw NTPPolicyException(LOCATION, "Incorrect number of csc:Comp in policy, found: " +
                                               std::to_string(compAttr.size()) + ", expected: 1");
        }
        auto comp = compAttr.at(0);
        if (comp.value("policyType") != "24") {
            throw NTPPolicyException(LOCATION, "NTP Policy not type 24");
        }
        revId_ = comp.value("RevID", "unknown");

        exclusions_ = extractExclusions(attributeMap.value());
    }
}

std::optional<Common::XmlUtilities::AttributesMap> NTPPolicy::openPolicy(const std::string& policy)
{
    std::optional<Common::XmlUtilities::AttributesMap> attributeMap;
    try
    {
        attributeMap = Common::XmlUtilities::parseXml(policy);
        return attributeMap;
    }
    catch (const Common::XmlUtilities::XmlUtilitiesException& ex)
    {
        //Should not report error if JSON
        try
        {
            nlohmann::json j = nlohmann::json::parse(policy);
            throw NTPPolicyIgnoreException(LOCATION, "Ignoring Json policy");
        }
        catch (const nlohmann::json::exception &)
        {
            std::stringstream ss;
            ss << "Unable to parse policy xml: " << policy;
            throw NTPPolicyException(LOCATION, ss.str());
        }
    }
}

const std::string& NTPPolicy::revId() const
{
    return revId_;
}

const std::vector<IsolationExclusion>& NTPPolicy::exclusions() const
{
    return exclusions_;
}
