// Copyright 2021-2023 Sophos Limited. All rights reserved.

#pragma once

#ifdef SPL_BAZEL
#include "common/livequery/include/OsquerySDK/OsquerySDK.h"
#else
#include "OsquerySDK/OsquerySDK.h"
#endif

#include "EventJournalWrapperImpl/IEventJournalReaderWrapper.h"

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
        // Read from event journal in chunks no greater than 50MB
        static const uint32_t MAX_MEMORY_THRESHOLD = 50000000;

        std::string getQueryId(QueryContextInterface& queryContext);
    };
}
