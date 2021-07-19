/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SophosAVDetectionTableGenerator.h"
#include "ThreatTypes.h"
#include "Logger.h"
#include "OsquerySDK/OsquerySDK.h"
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include <thirdparty/nlohmann-json/json.hpp>

namespace OsquerySDK
{
    OsquerySDK::TableRows SophosAVDetectionTableGenerator::GenerateData(
        const std::string& queryId,
        std::shared_ptr<Common::EventJournalWrapper::IEventJournalReaderWrapper> journalReader)
    {
        TableRows results;
        std::string currentJrl("");

        std::string installPath = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();
        std::string jrlFilePath = Common::FileSystem::join(installPath, "plugins/edr/var/jrl", queryId);

        if (!queryId.empty())
        {
            currentJrl = journalReader->getCurrentJRLForId(jrlFilePath);
        }

        std::vector<Common::EventJournalWrapper::Entry> entries;

        if (!currentJrl.empty())
        {
            entries = journalReader->getEntries({ Common::EventJournalWrapper::Subject::Detections }, currentJrl);
        }
        else
        {
            entries = journalReader->getEntries({ Common::EventJournalWrapper::Subject::Detections },0,0,0);
        }

        std::string newJrl("");

        for (auto& entry : entries)
        {
            // add bool check
            nlohmann::json jsonObject;
            std::pair<bool, Common::EventJournalWrapper::Detection> detectionResult = journalReader->decode(entry.data);
            if (detectionResult.first)
            {
                jsonObject = nlohmann::json::parse(detectionResult.second.data);
            }
            else
            {
                break;
            }
            TableRow r;

            r["time"] = std::to_string(entry.timestamp);

            if (jsonObject["details"].contains("time"))
            {
                r["time"] = to_string(jsonObject["details"]["time"]);
            }

            r["rowid"] = std::to_string(entry.producerUniqueID);

            r["query_id"] = queryId;

            std::string defaultRaw = detectionResult.second.data;
            r["raw"] = defaultRaw;

            nlohmann::json items = jsonObject["items"];

            for (auto item : items)
            {
                if (item["primary"] == true)
                {
                    r["primary_item"] = to_string(item);
                    r["primary_item_type"] = THREAT_ITEM_TYPE_MAP.find(item["type"])->second;
                    r["primary_item_name"] = item["path"];
                    r["detection_thumbprint"] = item["sha256"];
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
                r["detection_name"] = jsonObject["detectionName"]["full"];
            }
            else
            {
                r["detection_name"] = jsonObject["detectionName"]["short"];
            }

            r["threat_source"] = THREAT_SOURCE_MAP.find(jsonObject["threatSource"])->second;

            r["threat_type"] = THREAT_TYPE_MAP.find(jsonObject["threatType"])->second;

            r["sid"] = "";

            r["monitor_mode"] = "0";
            if (jsonObject.contains("monitorMode"))
            {
                r["monitor_mode"] = to_string(jsonObject["monitor_mode"]);
            }

            results.push_back(std::move(r));
            newJrl = entry.jrl;
        }

        if(!queryId.empty())
        {
            journalReader->updateJrl(jrlFilePath, newJrl);
        }

        return results;
    }
}