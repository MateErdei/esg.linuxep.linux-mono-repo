/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "CloudMetadataConverters.h"

#include <nlohmann/json.hpp>
#include <regex>
#include <sstream>

namespace Common::OSUtilitiesImpl
{
    std::string CloudMetadataConverters::parseAwsMetadataJson(std::string& responseBody)
    {
        try
        {
            nlohmann::json awsInfoJson = nlohmann::json::parse(responseBody);
            std::stringstream result;
            result << "<aws>"
                   << "<region>" << awsInfoJson["region"].get<std::string>() << "</region>"
                   << "<accountId>" << awsInfoJson["accountId"].get<std::string>() << "</accountId>"
                   << "<instanceId>" << awsInfoJson["instanceId"].get<std::string>() << "</instanceId>"
                   << "</aws>";

            return result.str();
        }
        catch (std::exception& e) // Catches JSON parse exception
        {
            return "";
        }
    }

    std::string CloudMetadataConverters::parseAzureMetadataJson(std::string& responseBody)
    {
        try
        {
            nlohmann::json azureInfoJson = nlohmann::json::parse(responseBody);
            std::stringstream result;
            result << "<azure>"
                   << "<vmId>" << azureInfoJson["compute"]["vmId"].get<std::string>() << "</vmId>"
                   << "<vmName>" << azureInfoJson["compute"]["name"].get<std::string>() << "</vmName>"
                   << "<resourceGroupName>" << azureInfoJson["compute"]["resourceGroupName"].get<std::string>()
                   << "</resourceGroupName>"
                   << "<subscriptionId>" << azureInfoJson["compute"]["subscriptionId"].get<std::string>()
                   << "</subscriptionId>"
                   << "</azure>";

            return result.str();
        }
        catch (std::exception& e) // Catches JSON parse exception
        {
            return "";
        }
    }

    std::string CloudMetadataConverters::parseGcpMetadata(std::map<std::string, std::string> metadataValues)
    {
        std::stringstream result;
        result << "<google>"
               << "<hostname>" << metadataValues["hostname"] << "</hostname>"
               << "<id>" << metadataValues["id"] << "</id>"
               << "<zone>" << metadataValues["zone"] << "</zone>"
               << "</google>";

        return result.str();
    }

    bool CloudMetadataConverters::verifyGoogleId(const std::string& id)
    {
        // verify it's a sensible length, the IDs are not very long
        if (id.length() > 100)
        {
            return false;
        }

        // Verify it contains characters we expect.
        // Currently, the ID should be just numbers but to be on the safe side in case the format
        // changes we'll allow alphanumerics with hyphens, which will still filter out most incorrect data but
        // won't leave our installer broken on GC if they tweak the format.
        if (!std::regex_match(id, std::regex(R"([a-zA-Z\-0-9]+)")))
        {
            return false;
        }

        return true;
    }

    std::string CloudMetadataConverters::parseOracleMetadataJson(std::string& responseBody)
    {
        try
        {
            nlohmann::json oracleInfoJson = nlohmann::json::parse(responseBody);
            std::stringstream result;
            result << "<oracle>"
                   << "<region>" << oracleInfoJson["region"].get<std::string>() << "</region>"
                   << "<availabilityDomain>" << oracleInfoJson["availabilityDomain"].get<std::string>()
                   << "</availabilityDomain>"
                   << "<compartmentId>" << oracleInfoJson["compartmentId"].get<std::string>() << "</compartmentId>"
                   << "<displayName>" << oracleInfoJson["displayName"].get<std::string>() << "</displayName>"
                   << "<hostname>" << oracleInfoJson["hostname"].get<std::string>() << "</hostname>"
                   << "<state>" << oracleInfoJson["state"].get<std::string>() << "</state>"
                   << "<instanceId>" << oracleInfoJson["id"].get<std::string>() << "</instanceId>"
                   << "</oracle>";

            return result.str();
        }
        catch (std::exception& e) // Catches JSON parse exception
        {
            return "";
        }
    }
} // namespace Common::OSUtilitiesImpl