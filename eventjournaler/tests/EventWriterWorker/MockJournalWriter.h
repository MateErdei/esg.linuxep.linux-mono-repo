// Copyright 2021 Sophos Limited. All rights reserved.

#pragma once

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#ifdef SPL_BAZEL
#include "EventJournal/IEventJournalWriter.h"
#else
#include "modules/EventJournal/IEventJournalWriter.h"
#endif

#include <gmock/gmock.h>

using namespace ::testing;
using namespace EventJournal;

class MockJournalWriter : public EventJournal::IEventJournalWriter
{
public:
    MockJournalWriter() = default;
    MOCK_METHOD2(insert, void(Subject, const std::vector<uint8_t>&));
    MOCK_CONST_METHOD2(readFileInfo, bool(const std::string&, FileInfo&));
    MOCK_METHOD1(pruneTruncatedEvents, void(const std::string&));
};
