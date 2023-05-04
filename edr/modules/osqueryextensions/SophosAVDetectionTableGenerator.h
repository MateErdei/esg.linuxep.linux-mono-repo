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
        // Read from event journal in chunks no greater than 10Mb
        static const uint32_t MAX_MEMORY_THRESHOLD = 10000000;

        std::string getQueryId(QueryContextInterface& queryContext);
    };
}
