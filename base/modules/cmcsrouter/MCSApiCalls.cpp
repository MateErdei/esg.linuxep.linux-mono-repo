// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "MCSApiCalls.h"

#include "AgentAdapter.h"
#include "Config.h"
#include "Logger.h"

#include "Common/HttpRequests/IHttpRequester.h"
#include "Common/ObfuscationImpl/Base64.h"
#include "Common/UtilityImpl/StringUtils.h"
#include "Common/XmlUtilities/AttributesMap.h"

#include <iostream>
#include <json.hpp>
#include <sstream>
#include <thread>

namespace
{
    template<class CharacterType, class Traits, class Rep, class Period>
    std::basic_ostream<CharacterType, Traits>& operator<<(
        std::basic_ostream<CharacterType, Traits>& os,
        const std::chrono::duration<Rep, Period>& d)
    {
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(d);
        os << std::to_string(seconds.count()) << " seconds";
        return os;
    }
} // namespace

namespace MCS
{
    std::map<std::string, std::string> MCSApiCalls::getAuthenticationInfo(
        const std::shared_ptr<MCS::MCSHttpClient>& client)
    {
        std::map<std::string, std::string> list;

        Common::HttpRequests::Headers requestHeaders;
        requestHeaders.insert({ "Content-Type", "application/xml; charset=utf-8" });
        Common::HttpRequests::Response response = client->sendMessageWithIDAndRole(
            "/authenticate/endpoint/", Common::HttpRequests::RequestType::POST, requestHeaders);
        if (response.status == 200)
        {
            nlohmann::json j;
            try
            {
                j = nlohmann::json::parse(response.body);
            }
            catch (const nlohmann::json::parse_error& ex)
            {
                std::stringstream errorMessage;
                errorMessage << "Could not parse json: " << response.body << " with error: " << ex.what();
                LOGWARN(errorMessage.str());
            }
            list.insert({ "tenant_id", j.value("tenant_id", "") });
            list.insert({ "device_id", j.value("device_id", "") });
            list.insert({ "access_token", j.value("access_token", "") });
        }
        return list;
    }

    std::string MCSApiCalls::preregisterEndpoint(
        const std::shared_ptr<MCS::MCSHttpClient>& client,
        MCS::ConfigOptions& configOptions,
        const std::string& statusXml,
        const std::string& proxy)
    {
        client->setVersion(configOptions.config[MCS::MCS_PRODUCT_VERSION]);

        if (!configOptions.config[MCS::MCS_CA_OVERRIDE].empty())
        {
            client->setCertPath(configOptions.config[MCS::MCS_CA_OVERRIDE]);
        }
        else
        {
            client->setCertPath(configOptions.config[MCS::MCS_CERT]);
        }

        client->setProxyInfo(
            proxy, configOptions.config[MCS::MCS_PROXY_USERNAME], configOptions.config[MCS::MCS_PROXY_PASSWORD]);
        Common::HttpRequests::Response response =
            client->sendPreregistration(statusXml, configOptions.config[MCS::MCS_CUSTOMER_TOKEN]);
        return response.body;
    }

    bool MCSApiCalls::registerEndpoint(
        const std::shared_ptr<MCS::MCSHttpClient>& client,
        MCS::ConfigOptions& configOptions,
        const std::string& statusXml,
        const std::string& proxy)
    {
        client->setID(configOptions.config[MCS::MCS_ID]);
        client->setPassword(configOptions.config[MCS::MCS_PASSWORD]);
        client->setVersion(configOptions.config[MCS::MCS_PRODUCT_VERSION]);

        if (!configOptions.config[MCS::MCS_CA_OVERRIDE].empty())
        {
            client->setCertPath(configOptions.config[MCS::MCS_CA_OVERRIDE]);
        }
        else
        {
            client->setCertPath(configOptions.config[MCS::MCS_CERT]);
        }
        std::stringstream connectionInfo;
        if (proxy.empty())
        {
            connectionInfo << " going direct";
        }
        else
        {
            connectionInfo << " via proxy " << proxy;
        }
        client->setProxyInfo(
            proxy, configOptions.config[MCS::MCS_PROXY_USERNAME], configOptions.config[MCS::MCS_PROXY_PASSWORD]);
        Common::HttpRequests::Response response =
            client->sendRegistration(statusXml, configOptions.config[MCS::MCS_TOKEN]);
        if (response.errorCode == Common::HttpRequests::ResponseErrorCode::OK)
        {
            if (response.status == Common::HttpRequests::HTTP_STATUS_OK)
            {
                std::string messageBody = Common::ObfuscationImpl::Base64::Decode(response.body);
                std::vector<std::string> responseValues =
                    Common::UtilityImpl::StringUtils::splitString(messageBody, ":");
                if (responseValues.size() == 2)
                {
                    // Note that updating the configOptions here should be propagated back to the caller as it is all
                    // passed by reference.
                    configOptions.config[MCS::MCS_ID] = responseValues[0];       // endpointId
                    configOptions.config[MCS::MCS_PASSWORD] = responseValues[1]; // MCS Password
                    return true;
                }
            }
            else
            {
                LOGWARN(
                    "Unexpected status returned during registration" << connectionInfo.str() << ": " << response.status
                                                                     << ".");
            }
        }
        else
        {
            LOGWARN("Connection error during registration" << connectionInfo.str() << ": " << response.error);
        }
        return false;
    }

    std::optional<std::string> MCSApiCalls::getPolicy(
        const std::shared_ptr<MCS::MCSHttpClient>& client,
        const std::string& appId,
        int policyId,
        duration_t timeout,
        duration_t pollInterval)
    {
        using clock_t = std::chrono::steady_clock;
        auto start = clock_t::now();
        auto end = start + timeout;
        Common::HttpRequests::Response response;
        std::optional<std::string> commandPolicyId;
        size_t totalCommands = 0;
        while (clock_t::now() < end && !commandPolicyId.has_value())
        {
            // Send get commands for APPSPROXY
            std::ostringstream pollUrl;
            pollUrl << "/commands/applications/APPSPROXY;" << appId << "/endpoint/";
            response = client->sendMessageWithID(pollUrl.str(), Common::HttpRequests::RequestType::GET);

            LOGDEBUG("Response to poll: " << response.body);
            // Parse XML
            auto xml = Common::XmlUtilities::parseXml(response.body);
            auto commands = xml.entitiesThatContainPath("ns:commands/command", false);
            LOGDEBUG("Found " << commands.size() << " commands");
            totalCommands += commands.size();

            std::vector<std::string> toDelete;
            for (const auto& command : commands)
            {
                auto id = xml.lookup(command + "/id").contents();
                toDelete.push_back(id);
                auto app = xml.lookup(command + "/appId").contents();
                if (app != "APPSPROXY")
                {
                    LOGDEBUG("Ignoring command for " << app);
                    continue;
                }
                auto assignmentsString = xml.lookup(command + "/body").contents();
                LOGDEBUG("Assignment:" << assignmentsString);
                auto assignmentsXml = Common::XmlUtilities::parseXml(assignmentsString);
                auto assignments = assignmentsXml.entitiesThatContainPath("ns:policyAssignments/policyAssignment");
                for (const auto& assignment : assignments)
                {
                    auto appAttr = assignmentsXml.lookup(assignment + "/appId");
                    if (appAttr.contents() != appId)
                    {
                        continue;
                    }
                    auto policyType = appAttr.value("policyType");
                    if (policyType != std::to_string(policyId))
                    {
                        continue;
                    }
                    commandPolicyId = assignmentsXml.lookup(assignment + "/policyId").contents();
                    break;
                }
            }

            // delete commands
            if (!toDelete.empty())
            {
                std::ostringstream deleteUrl;
                deleteUrl << "/commands/endpoint/" << client->getID() << "/";
                bool first = true;
                for (const auto& id : toDelete)
                {
                    if (first)
                    {
                        first = false;
                    }
                    else
                    {
                        deleteUrl << ';';
                    }
                    deleteUrl << id;
                }
                LOGDEBUG("Delete commands with url " << deleteUrl.str());
                client->sendMessage(deleteUrl.str(), Common::HttpRequests::RequestType::DELETE);
            }

            if (commandPolicyId.has_value())
            {
                break;
            }

            // Repeat after delay
            auto duration = clock_t::now() - start;
            LOGINFO("Waiting for policy after " << totalCommands << " commands after " << duration);
            std::this_thread::sleep_for(pollInterval);
        }

        if (commandPolicyId.has_value())
        {
            // fetch last policy in command
            std::stringstream url;
            url << "/policy/application/" << appId << "/" << commandPolicyId.value();
            LOGDEBUG("Fetching policy from " << url.str());
            response = client->sendMessage(url.str(), Common::HttpRequests::RequestType::GET);
            auto duration = clock_t::now() - start;
            LOGINFO("Got policy after " << duration);
            return response.body;
        }
        else
        {
            auto duration = clock_t::now() - start;
            LOGERROR("Failed to get policy after " << duration);
        }
        return std::nullopt;
    }
} // namespace MCS
