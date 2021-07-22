/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "modules/EventJournal/IEventJournalWriter.h"

#include <gmock/gmock.h>

using namespace ::testing;
using namespace EventJournal;

class MockJournalWriter : public EventJournal::IEventJournalWriter
{
public:
    MockJournalWriter() = default;
    MOCK_METHOD2(insert, void(Subject, const std::vector<uint8_t>&));
};
