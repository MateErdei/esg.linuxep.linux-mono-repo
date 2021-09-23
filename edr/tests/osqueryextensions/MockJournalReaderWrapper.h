/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once
#include <modules/EventJournalWrapperImpl/IEventJournalReaderWrapper.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

class MockJournalReaderWrapper : public virtual Common::EventJournalWrapper::IEventJournalReaderWrapper
{
public:
    MOCK_METHOD1(getCurrentJRLForId, std::string (const std::string&));
    MOCK_METHOD2(updateJrl, void (const std::string&, const std::string&));
    MOCK_METHOD1(getCurrentJRLAttemptsForId, u_int32_t (const std::string&));
    MOCK_METHOD2(updateJRLAttempts, void (const std::string&, const u_int32_t attempts));
    MOCK_METHOD1(clearJRLFile, void (const std::string&));
    MOCK_METHOD4(getEntries, std::vector<Common::EventJournalWrapper::Entry>(std::vector<Common::EventJournalWrapper::Subject>, const std::string&, uint32_t, bool&));
    MOCK_METHOD5(getEntries, std::vector<Common::EventJournalWrapper::Entry> (std::vector<Common::EventJournalWrapper::Subject> subjectFilter, uint64_t startTime, uint64_t endTime, uint32_t, bool&));
    MOCK_METHOD1(decode, std::pair<bool, Common::EventJournalWrapper::Detection> (const std::vector<uint8_t>& data));
};
