// Copyright 2021-2023 Sophos Limited. All rights reserved.
#pragma once

#include "EventJournalWrapperImpl/IEventJournalReaderWrapper.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

class MockJournalReaderWrapper : public virtual Common::EventJournalWrapper::IEventJournalReaderWrapper
{
public:
    MOCK_METHOD(std::string, getCurrentJRLForId, (const std::string&));
    MOCK_METHOD(void, updateJrl, (const std::string&, const std::string&));
    MOCK_METHOD(u_int32_t, getCurrentJRLAttemptsForId, (const std::string&));
    MOCK_METHOD(void, updateJRLAttempts, (const std::string&, const u_int32_t attempts));
    MOCK_METHOD(void, clearJRLFile, (const std::string&));
    MOCK_METHOD(std::vector<Common::EventJournalWrapper::Entry>, getEntries, (std::vector<Common::EventJournalWrapper::Subject>, const std::string&, uint32_t, bool&));
    MOCK_METHOD(std::vector<Common::EventJournalWrapper::Entry>, getEntries, (std::vector<Common::EventJournalWrapper::Subject> subjectFilter, uint64_t startTime, uint64_t endTime, uint32_t, bool&));
    MOCK_METHOD((std::pair<bool, Common::EventJournalWrapper::Detection>), decode, (const std::vector<uint8_t>& data));
};
