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
    MOCK_METHOD2(getEntries, std::vector<Common::EventJournalWrapper::Entry>(std::vector<Common::EventJournalWrapper::Subject>, const std::string&));
    MOCK_METHOD3(getEntries, std::vector<Common::EventJournalWrapper::Entry> (std::vector<Common::EventJournalWrapper::Subject> subjectFilter, uint64_t startTime, uint64_t endTime));
    MOCK_METHOD1(decode, std::pair<bool, Common::EventJournalWrapper::Detection> (const std::vector<uint8_t>& data));
};
