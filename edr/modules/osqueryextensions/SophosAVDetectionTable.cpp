/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SophosAVDetectionTable.h"
#include "ThreatTypes.h"


#include "Logger.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/UtilityImpl/FileUtils.h>
#include <json/value.h>
#include <modules/pluginimpl/ApplicationPaths.h>
#include <thirdparty/nlohmann-json/json.hpp>

#include <modules/EventJournalWrapperImpl/EventJournalWrapper.h>

#include <iostream>

namespace OsquerySDK
{
    std::string SophosAVDetectionTable::GetName() { return "sophos_detections_journal"; }

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
                OsquerySDK::TableColumn{"primary_item_spid", TEXT_TYPE, ColumnOptions::DEFAULT}
            };
    }

    TableRows SophosAVDetectionTable::Generate(QueryContextInterface& queryContext)
    {
        LOGINFO("Generating data for Detections Table");
        TableRows results;

        std::string installPath = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();
//        auto fileSystem = Common::FileSystem::fileSystem();


        std::string journalDir = Common::FileSystem::join(installPath,"plugins/edr/etc");
        std::string journalFile = Common::FileSystem::join(journalDir,"testfile");
        Common::EventJournalWrapper::Reader journalReader("/tmp/event_journal");

        std::set<std::string> queryIdConstraint = queryContext.GetConstraints("query_id", OsquerySDK::EQUALS);
        std::string queryId;
        if (queryIdConstraint.size() == 1)
        {
            queryId = queryIdConstraint.begin()->c_str();
        }

        std::string currentJrl("");

        std::string jrlFilePath = Common::FileSystem::join(installPath, "plugins/edr/var/jrl", queryId);

        if(!queryId.empty())
        {
            currentJrl = journalReader.getCurrentJRLForId(jrlFilePath);
        }

        std::vector< Common::EventJournalWrapper::Entry> entries;

        if(!currentJrl.empty())
        {
            entries = journalReader.getEntries({Common::EventJournalWrapper::Subject::Detections}, currentJrl);
        }
        else
        {
            entries = journalReader.getEntries({Common::EventJournalWrapper::Subject::Detections});
        }

        entries =
            journalReader.getEntries({Common::EventJournalWrapper::Subject::Detections}, currentJrl);

        std::string newJrl("");

        for(auto& entry : entries)
        {
            Common::EventJournalWrapper::Detection detection;

            // add bool check
            journalReader.decode(entry.data, detection);
            nlohmann::json jsonObject = nlohmann::json::parse( detection.data);

            TableRow r;

            r["time"] = std::to_string(entry.timestamp);

            if (jsonObject["details"].contains("time"))
            {
                r["time"] = to_string(jsonObject["details"]["time"]);
            }

            r["rowid"] = std::to_string(entry.producerUniqueID);
            LOGINFO("Query ID: " << queryId);
            // set this to default, as we do not know what it is
            r["query_id"] = queryId;

            std::string defaultRaw = detection.data;
            r["raw"] = defaultRaw;

            nlohmann::json items = jsonObject["items"];

            for (auto item : items)
            {
                if (item["primary"] == true)
                {
                    r["primary_item"] = to_string(item);
                    r["primary_item_type"] = to_string(item["type"]); // int, maybe convert using a map
                    r["primary_item_name"] = item["path"]; // we have a path, ask if there is more than just it needed
                    r["detection_thumbprint"] = item["sha256"]; // what value is here, we have sha1 and sha256 hashes
                    r["primary_item_spid"] = "";
                    if (item.contains("spid"))
                    {
                        r["primary_item_spid"] = item["spid"];
                    }
                    break;
                }
            }
            if (jsonObject["detectionName"].contains("full"))
            {
                r["detection_name"] = jsonObject["detectionName"]["full"]; // what value does it have to be here
            }
            else
            {
                r["detection_name"] = jsonObject["detectionName"]["short"]; // what value does it have to be here
            }

            r["threat_source"] = THREAT_SOURCE_MAP.find(jsonObject["threatSource"])->second; // we have, but is an int

            r["threat_type"] = THREAT_TYPE_MAP.find(jsonObject["threatType"])->second; // we have, but is an int

            r["sid"] = ""; // we do not have

            r["monitor_mode"] = "0"; // optional, ask whether it is a field on its own, or a json field
            if (jsonObject.contains("monitorMode"))
            {
                r["monitor_mode"] = to_string(jsonObject["monitor_mode"]);
            }

            results.push_back(std::move(r));
            newJrl = entry.jrl;
        }
        if(!newJrl.empty() && currentJrl != newJrl)
        {
            journalReader.updateJrl(jrlFilePath, newJrl);
        }

        return results;
    }









    std::string SophosAVDetectionTable::getSampleJson()
    {
        return "{\n"
               "    \"details\": {\n"
               "        \"time\": 123123123,\n"
               "        \"certificates\": {},\n"
               "        \"fileMetadata\": {},\n"
               "        \"filePath\": \"/opt/testdir/file.sh\",\n"
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
               "            \"path\": \"/opt/testdir/file.sh\",\n"
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

