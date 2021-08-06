/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "OsquerySDK/OsquerySDK.h"

#include <modules/EventJournalWrapperImpl/IEventJournalReaderWrapper.h>
namespace OsquerySDK
{
    class SophosAVDetectionTableGenerator
    {
    public:
        SophosAVDetectionTableGenerator() = default;
        ~SophosAVDetectionTableGenerator() = default;

        OsquerySDK::TableRows GenerateData(
            QueryContextInterface& queryContext,
            std::shared_ptr<Common::EventJournalWrapper::IEventJournalReaderWrapper> journalReader);
    private:
        std::string getQueryId(QueryContextInterface& queryContext);
    };
}
