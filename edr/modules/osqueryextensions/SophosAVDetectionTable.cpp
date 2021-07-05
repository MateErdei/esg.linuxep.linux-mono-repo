/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SophosAVDetectionTable.h"

#include "Logger.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/UtilityImpl/FileUtils.h>
#include <json/value.h>
#include <thirdparty/nlohmann-json/json.hpp>

#include <iostream>

namespace OsquerySDK
{
    std::vector<TableColumn> SophosAVDetectionTable::GetColumns()
    {
        return
            {
                OsquerySDK::TableColumn{"rowid", UNSIGNED_BIGINT_TYPE, ColumnOptions::HIDDEN},
                OsquerySDK::TableColumn{"time", UNSIGNED_BIGINT_TYPE, ColumnOptions::INDEX},
                OsquerySDK::TableColumn{"raw", TEXT_TYPE, ColumnOptions::DEFAULT},
                OsquerySDK::TableColumn{"query_id", TEXT_TYPE, ColumnOptions::DEFAULT},
                OsquerySDK::TableColumn{"detection_name", TEXT_TYPE, ColumnOptions::DEFAULT},
                OsquerySDK::TableColumn{"detection_thumbprint", TEXT_TYPE, ColumnOptions::DEFAULT},
                OsquerySDK::TableColumn{"threat_source", TEXT_TYPE, ColumnOptions::DEFAULT},
                OsquerySDK::TableColumn{"threat_type", TEXT_TYPE, ColumnOptions::DEFAULT},
                OsquerySDK::TableColumn{"sid", TEXT_TYPE, ColumnOptions::DEFAULT},
                OsquerySDK::TableColumn{"monitor_mode", INTEGER_TYPE, ColumnOptions::DEFAULT},
                OsquerySDK::TableColumn{"primary_item", TEXT_TYPE, ColumnOptions::DEFAULT},
                OsquerySDK::TableColumn{"primary_item_type", TEXT_TYPE, ColumnOptions::DEFAULT},
                OsquerySDK::TableColumn{"primary_item_name", TEXT_TYPE, ColumnOptions::DEFAULT},
                OsquerySDK::TableColumn{"primary_item_spid", TEXT_TYPE, ColumnOptions::DEFAULT},
            };
    }

    std::string SophosAVDetectionTable::GetName() { return "sophos_detections_journal"; }

    TableRows SophosAVDetectionTable::Generate(QueryContextInterface& /*request*/)
    {
        LOGINFO("Generating data for Detections Table");
        TableRows results;
//        nlohmann::json columnMetaData = nlohmann::json::array();
//        const auto& headers = queryResponse.data().columnHeaders();
        std::string jsonString = SophosAVDetectionTable::getSampleJson();
        nlohmann::json jsonObject = nlohmann::json::parse(jsonString);

        TableRow r;

        //when time is not present, we use the current time -  we do not have this in the trigger context
        unsigned int defaultTime = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        r["time"] = std::to_string(defaultTime);

        //we set the rowid to an increasing counter (the counter will increase with each event in the journal read)
        //we do not have this in the trigger context
        //need to implement actually reading the journal events
        unsigned int defaultRowID = 0;
        r["rowid"] = std::to_string(defaultRowID);


        //set this to default, as we do not know what it is
        std::string defaultQueryID = "";
        r["query_id"] = defaultQueryID;

        std::string defaultRaw = SophosAVDetectionTable::getSampleJson();
        r["raw"] = defaultRaw;

        nlohmann::json items = jsonObject["items"];

        for (auto it : items)
        {
            auto item = it;
            if (item["primary"] == true)
            {
                r["primary_item"] = to_string(item);
                r["primary_item_type"] = to_string(item["type"]); // int, maybe convert using a map
                r["primary_item_name"] = to_string(item["path"]);; // we have a path, ask if there is more than just it needed
                r["detection_thumbprint"] = to_string(item["sha256"]); // what value is here, we have sha1 and sha256 hashes
                r["primary_item_spid"] = "";
                if(item.contains("spid"))
                {
                    r["primary_item_spid"] = to_string(item["spid"]);
                }
                break;
            }
        }
        if (jsonObject["detectionName"].contains("full"))
        {
            r["detection_name"] = to_string(jsonObject["detectionName"]["full"]); // what value does it have to be here
        }
        else
        {
            r["detection_name"] = to_string(jsonObject["detectionName"]["short"]); // what value does it have to be here
        }

        r["threat_source"] = to_string(jsonObject["threatSource"]); // we have, but is an int

        r["threat_type"] = to_string(jsonObject["threatType"]); // we have, but is an int

        r["sid"] = ""; // we do not have

        r["monitor_mode"] = "0";  // optional, ask whether it is a field on its own, or a json field
        if (jsonObject.contains("monitorMode"))
        {
            r["monitor_mode"] = to_string(jsonObject["monitor_mode"]);
        }

//        std::string installPath = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();
//        auto fileSystem = Common::FileSystem::fileSystem();
//
//        std::string mcsConfigPath = Common::FileSystem::join(installPath,"base/etc/sophosspl/mcs.config");
//        std::string mcsPolicyConfigPath = Common::FileSystem::join(installPath,"base/etc/sophosspl/mcs_policy.config");
//
//        if (fileSystem->isFile(mcsConfigPath))
//        {
//            std::pair<std::string,std::string> value = Common::UtilityImpl::FileUtils::extractValueFromFile(mcsConfigPath,"MCSID");
//            r["endpoint_id"] = value.first;
//            if (value.first.empty() && !value.second.empty())
//            {
//                LOGWARN("Failed to read MCSID configuration from config file: " << mcsConfigPath << " with error: " << value.second);
//            }
//        }
//        else
//        {
//            LOGWARN("Failed to read MCSID configuration from config file: " << mcsConfigPath << " with error: file doesn't exist");
//        }
//
//        if (fileSystem->isFile(mcsPolicyConfigPath))
//        {
//            std::pair<std::string,std::string> value = Common::UtilityImpl::FileUtils::extractValueFromFile(mcsPolicyConfigPath,"customerId");
//            r["customer_id"] = value.first;
//            if (value.first.empty() && !value.second.empty())
//            {
//                LOGWARN("Failed to read customerID configuration from config file: " << mcsPolicyConfigPath << " with error: " << value.second);
//            }
//        }
//        else
//        {
//            LOGWARN("Failed to read customerID configuration from config file: " << mcsPolicyConfigPath << " with error: file doesn't exist");
//        }
        results.push_back(std::move(r));
        return results;
    }









    std::string SophosAVDetectionTable::getSampleJson()
    {
        return "{\n"
               "    \"details\": {\n"
               "        \"certificates\": {},\n"
               "        \"fileMetadata\": {},\n"
               "        \"filePath\": \"C:\\\\Users\\\\Administrator\\\\Desktop\\\\HighScore.exe\",\n"
               "        \"fileSize\": 660,\n"
               "        \"globalReputation\": 62,\n"
               "        \"localReputation\": -1,\n"
               "        \"localReputationConfigVersion\": \"e9f53340ca6598d87ba24a832311b9c61fb486d9bbf9b25c53b6c3d4fdd7578c\",\n"
               "        \"localReputationSampleRate\": 0,\n"
               "        \"mlScoreMain\": 100,\n"
               "        \"mlScorePua\": 100,\n"
               "        \"mlSuppressed\": false,\n"
               "        \"remote\": false,\n"
               "        \"reputationSource\": \"SXL4_LOOKUP\",\n"
               "        \"savScanResult\": \"\",\n"
               "        \"scanEvalReason\": \"PINNED_CACHE_BLOCK_NOTIFY\",\n"
               "        \"sha1FileHash\": \"46763aa950d6f2a3615cc63226afb5180b5a229b\",\n"
               "        \"sha256FileHash\": \"c88e20178a82af37a51b030cb3797ed144126cad09193a6c8c7e95957cf9c3f9\",\n"
               "        \"sting20Enabled\": false\n"
               "    },\n"
               "    \"detectionName\": {\n"
               "        \"short\": \"ML/PE-A\"\n"
               "    },\n"
               "    \"flags\": {\n"
               "        \"isSuppressible\": false,\n"
               "        \"isUserVisible\": true,\n"
               "        \"sendVdlTelemetry\": true,\n"
               "        \"useClassicTelemetry\": true,\n"
               "        \"useSavScanResults\": true\n"
               "    },\n"
               "    \"items\": {\n"
               "        \"1\": {\n"
               "            \"certificates\": \"{\\\"details\\\": {}, \\\"isSigned\\\": false, \\\"signerInfo\\\": []}\",\n"
               "            \"cleanUp\": true,\n"
               "            \"isPeFile\": true,\n"
               "            \"path\": \"C:\\\\Users\\\\Administrator\\\\Desktop\\\\HighScore.exe\",\n"
               "            \"primary\": true,\n"
               "            \"remotePath\": false,\n"
               "            \"sha256\": \"c88e20178a82af37a51b030cb3797ed144126cad09193a6c8c7e95957cf9c3f9\",\n"
               "            \"type\": 1\n"
               "        }\n"
               "    },\n"
               "    \"threatSource\": 0,\n"
               "    \"threatType\": 1\n"
               "}";
    }



}

