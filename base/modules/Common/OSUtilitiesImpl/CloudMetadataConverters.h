/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <sstream>
#include <json.hpp>

namespace Common::OSUtilitiesImpl
{
    class CloudMetadataConverters
    {
    public:
        /**
         * @param responseBody - Json returned from aws metadata request as string
         * @return result - xmlStatus compatible xml as string
         */
        static std::string parseAwsMetadataJson(std::string& responseBody)
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

        /**
         * @param responseBody - Json returned from azure metadata request as string
         * @return result - xmlStatus compatible xml as string
         */
        static std::string parseAzureMetadataJson(std::string& responseBody)
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

        /**
         * @param responseBody - Json returned from gcp metadata request as string
         * @return result - xmlStatus compatible xml as string
         */
        static std::string parseGcpMetadata(std::map<std::string, std::string> metadataValues)
        {
            std::stringstream result;
            result << "<google>"
                   << "<hostname>" << metadataValues["hostname"] << "</hostname>"
                   << "<id>" << metadataValues["id"] << "</id>"
                   << "<zone>" << metadataValues["zone"] << "</zone>"
                   << "</google>";

            return result.str();
        }

        /**
         * @param responseBody - Json returned from oracle metadata request as string
         * @return result - xmlStatus compatible xml as string
         */
        static std::string parseOracleMetadataJson(std::string& responseBody)
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
    };
}