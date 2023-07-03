/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <string>
#include <map>

namespace Common::OSUtilitiesImpl
{
    class CloudMetadataConverters
    {
    public:
        /**
         * @param responseBody - Json returned from aws metadata request as string
         * @return result - xmlStatus compatible xml as string
         */
        static std::string parseAwsMetadataJson(std::string& responseBody);

        /**
         * @param responseBody - Json returned from azure metadata request as string
         * @return result - xmlStatus compatible xml as string
         */
        static std::string parseAzureMetadataJson(std::string& responseBody);

        /**
         * @param responseBody - Json returned from gcp metadata request as string
         * @return result - xmlStatus compatible xml as string
         */
        static std::string parseGcpMetadata(std::map<std::string, std::string> metadataValues);

        [[nodiscard]] static bool verifyGoogleId(const std::string& id);

        /**
         * @param responseBody - Json returned from oracle metadata request as string
         * @return result - xmlStatus compatible xml as string
         */
        static std::string parseOracleMetadataJson(std::string& responseBody);
    };
}