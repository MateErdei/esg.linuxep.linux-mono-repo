/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SophosAVDetectionTableGenerator.h"

#include "Logger.h"
#include "ThreatTypes.h"
#include "TimeConstraintHelpers.h"

#include "OsquerySDK/OsquerySDK.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include <thirdparty/nlohmann-json/json.hpp>

namespace OsquerySDK
{
    OsquerySDK::TableRows SophosAVDetectionTableGenerator::GenerateData(
        QueryContextInterface& queryContext,
        std::shared_ptr<Common::EventJournalWrapper::IEventJournalReaderWrapper> journalReader)
    {
        TableRows results;
        std::string currentJrl("");
        std::string queryId = getQueryId(queryContext);

        TimeConstraintHelpers timeConstraintHelpers;
        std::pair<uint64_t , uint64_t> queryTimeConstraints = timeConstraintHelpers.GetTimeConstraints(queryContext);

        std::string installPath = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();
        std::string jrlFilePath = Common::FileSystem::join(installPath, "plugins/edr/var/jrl", queryId);
        std::string jrlAttemptFilePath = Common::FileSystem::join(installPath, "plugins/edr/var/jrl_tracker", queryId);

        if (!queryId.empty())
        {
            currentJrl = journalReader->getCurrentJRLForId(jrlFilePath);
        }

        std::vector<Common::EventJournalWrapper::Entry> entries;
        bool moreEntriesAvailable = false;

        if (!currentJrl.empty())
        {
            entries = journalReader->getEntries({ Common::EventJournalWrapper::Subject::Detections }, currentJrl, MAX_EVENTS_PER_QUERY, moreEntriesAvailable);
        }
        else
        {
            entries = journalReader->getEntries({ Common::EventJournalWrapper::Subject::Detections }, queryTimeConstraints.first, queryTimeConstraints.second, MAX_EVENTS_PER_QUERY, moreEntriesAvailable);
        }

        if (moreEntriesAvailable)
        {
            if (queryId.empty())
            {
                throw std::runtime_error("Query returned too many events, please restrict search period to a shorter time period");
            }

            u_int32_t attempts = journalReader->getCurrentJRLAttemptsForId(jrlAttemptFilePath);
            attempts = attempts + 1;
            if (attempts > 10)
            {
                LOGWARN("Exceeded detection event limit for query ID " << queryId << " more than 10 times, resetting JRL");
                journalReader->clearJRLFile(jrlAttemptFilePath);
                journalReader->clearJRLFile(jrlFilePath);
            }
            else
            {
                journalReader->updateJRLAttempts(jrlAttemptFilePath,attempts);
            }
        }
        else if (!queryId.empty())
        {
            journalReader->clearJRLFile(jrlAttemptFilePath);
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

    std::string SophosAVDetectionTableGenerator::getQueryId(QueryContextInterface& queryContext)
    {
        std::set<std::string> queryIdConstraint = queryContext.GetConstraints("query_id", OsquerySDK::EQUALS);
        std::string queryId;

        if (queryIdConstraint.size() == 1)
        {
            queryId = queryIdConstraint.begin()->c_str();
        }

        return queryId;
    }
}