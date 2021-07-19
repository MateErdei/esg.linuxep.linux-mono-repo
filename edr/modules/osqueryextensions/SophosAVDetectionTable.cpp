/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SophosAVDetectionTable.h"
#include "SophosAVDetectionTableGenerator.h"
#include "Logger.h"
#include "ThreatTypes.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/UtilityImpl/FileUtils.h>
#include <json/value.h>
#include <modules/EventJournalWrapperImpl/IEventJournalReaderWrapper.h>
#include <modules/EventJournalWrapperImpl/EventJournalReaderWrapper.h>


#include <iostream>

namespace OsquerySDK
{
    std::string SophosAVDetectionTable::GetName()
    {
        return "sophos_detections_journal";
    }

    std::vector<TableColumn> SophosAVDetectionTable::GetColumns()
    {
        return { OsquerySDK::TableColumn { "rowid", UNSIGNED_BIGINT_TYPE, ColumnOptions::HIDDEN },
                 OsquerySDK::TableColumn { "time", UNSIGNED_BIGINT_TYPE, ColumnOptions::INDEX },
                 OsquerySDK::TableColumn { "raw", TEXT_TYPE, ColumnOptions::DEFAULT },
                 OsquerySDK::TableColumn { "query_id", TEXT_TYPE, ColumnOptions::DEFAULT },
                 OsquerySDK::TableColumn { "detection_name", TEXT_TYPE, ColumnOptions::DEFAULT },
                 OsquerySDK::TableColumn { "detection_thumbprint", TEXT_TYPE, ColumnOptions::DEFAULT },
                 OsquerySDK::TableColumn { "threat_source", TEXT_TYPE, ColumnOptions::DEFAULT },
                 OsquerySDK::TableColumn { "threat_type", TEXT_TYPE, ColumnOptions::DEFAULT },
                 OsquerySDK::TableColumn { "sid", TEXT_TYPE, ColumnOptions::DEFAULT },
                 OsquerySDK::TableColumn { "monitor_mode", INTEGER_TYPE, ColumnOptions::DEFAULT },
                 OsquerySDK::TableColumn { "primary_item", TEXT_TYPE, ColumnOptions::DEFAULT },
                 OsquerySDK::TableColumn { "primary_item_type", TEXT_TYPE, ColumnOptions::DEFAULT },
                 OsquerySDK::TableColumn { "primary_item_name", TEXT_TYPE, ColumnOptions::DEFAULT },
                 OsquerySDK::TableColumn { "primary_item_spid", TEXT_TYPE, ColumnOptions::DEFAULT } };
    }

    TableRows SophosAVDetectionTable::Generate(QueryContextInterface& queryContext)
    {
        LOGINFO("Generating data for Detections Table");
        TableRows results;

        std::string installPath = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();

        std::string journalDir = Common::FileSystem::join(installPath, "plugins/eventjournaler/data/event-journals");

        std::shared_ptr<Common::EventJournalWrapper::IEventJournalReaderWrapper> journalReader =
            std::make_shared<Common::EventJournalWrapper::Reader>(journalDir);

        std::set<std::string> queryIdConstraint = queryContext.GetConstraints("query_id", OsquerySDK::EQUALS);
        std::string queryId;

        if (queryIdConstraint.size() == 1)
        {
            queryId = queryIdConstraint.begin()->c_str();
        }
        SophosAVDetectionTableGenerator generator;
        return generator.GenerateData(queryId, journalReader);

    }
}

