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
    TableRows SophosAVDetectionTable::Generate(QueryContextInterface& queryContext)
    {
        LOGDEBUG("Generating data for Detections Table");
        TableRows results;

        std::string journalDir = Common::ApplicationConfiguration::applicationPathManager().getEventJournalsPath();

        std::shared_ptr<Common::EventJournalWrapper::IEventJournalReaderWrapper> journalReader =
            std::make_shared<Common::EventJournalWrapper::Reader>(journalDir);

        SophosAVDetectionTableGenerator generator;
        return generator.GenerateData(queryContext, journalReader);

    }
}

