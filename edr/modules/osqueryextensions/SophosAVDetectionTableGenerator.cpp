// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include "SophosAVDetectionTableGenerator.h"

#include "ConstraintHelpers.h"
#include "Logger.h"
#include "ThreatTypes.h"

#include "OsquerySDK/OsquerySDK.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include <nlohmann/json.hpp>

namespace OsquerySDK
{
    OsquerySDK::TableRows SophosAVDetectionTableGenerator::GenerateData(
        QueryContextInterface& queryContext,
        std::shared_ptr<Common::EventJournalWrapper::IEventJournalReaderWrapper> journalReader)
    {
        TableRows results;
        std::string currentJrl("");
        std::string queryId = getQueryId(queryContext);

        ConstraintHelpers timeConstraintHelpers;
        auto [start, end] =  timeConstraintHelpers.GetTimeConstraints(queryContext);
        LOGDEBUG("Constraint - start: " << start);
        LOGDEBUG("Constraint - end: " << end);
        std::string installPath = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();
        std::string jrlFilePath = Common::FileSystem::join(installPath, "plugins/edr/var/jrl", queryId);
        std::string jrlAttemptFilePath = Common::FileSystem::join(installPath, "plugins/edr/var/jrl_tracker", queryId);

        if (!queryId.empty())
        {
            currentJrl = journalReader->getCurrentJRLForId(jrlFilePath);
        }

        std::vector<Common::EventJournalWrapper::Entry> entries;
        bool moreEntriesAvailable = false;
        bool clearJrl = false;
        std::vector<Common::EventJournalWrapper::Subject> subjectFilter = { Common::EventJournalWrapper::Subject::Detections };

        if (!currentJrl.empty())
        {
            entries = journalReader->getEntries(subjectFilter, currentJrl, MAX_MEMORY_THRESHOLD, moreEntriesAvailable);
        }
        else
        {
            entries = journalReader->getEntries(subjectFilter, start, end, MAX_MEMORY_THRESHOLD, moreEntriesAvailable);
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
                clearJrl = true;
                journalReader->clearJRLFile(jrlAttemptFilePath);
                journalReader->clearJRLFile(jrlFilePath);
            }
            else
            {
                journalReader->updateJRLAttempts(jrlAttemptFilePath, attempts);
            }
        }
        else if (!queryId.empty())
        {
            journalReader->clearJRLFile(jrlAttemptFilePath);
        }

        std::string newJrl(currentJrl);

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
            if (jsonObject.contains("detectionThumbprint"))
            {
                r["detection_thumbprint"] = jsonObject["detectionThumbprint"];
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
            if (jsonObject.contains("sid"))
            {
                r["sid"] = jsonObject["sid"];
            }

            r["monitor_mode"] = "0";
            if (jsonObject.contains("monitorMode"))
            {
                r["monitor_mode"] = to_string(jsonObject["monitor_mode"]);
            }

            if (jsonObject.contains("quarantineSuccess"))
            {
                if (jsonObject["quarantineSuccess"])
                {
                    r["quarantine_success"] = "1";
                }
                else
                {
                    r["quarantine_success"] = "0";
                }
            }
            if (jsonObject.contains("avScanType"))
            {
                r["av_scan_type"] = jsonObject["avScanType"] == 201 ? "on_access":"on_demand";
                if (r["av_scan_type"] == "on_access")
                {
                    if (jsonObject.contains("pid"))
                    {
                        r["pid"] = to_string(jsonObject["pid"]);
                    }
                    if (jsonObject.contains("processPath"))
                    {
                        r["process_path"] = jsonObject["processPath"];
                    }
                }
            }

            results.push_back(std::move(r));
            newJrl = entry.jrl;
        }

        if(!queryId.empty() && !clearJrl && !newJrl.empty())
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