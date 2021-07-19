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
        OsquerySDK::TableRows GenerateData(
            const std::string& queryId,
            std::shared_ptr<Common::EventJournalWrapper::IEventJournalReaderWrapper> journalReader);
    };
}
