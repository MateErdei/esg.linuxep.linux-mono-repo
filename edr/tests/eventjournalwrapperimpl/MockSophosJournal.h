// Copyright 2021-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"

#include "EventJournalWrapperImpl/EventJournalTimeUtils.h"

#ifdef SPL_BAZEL
#include "journallib/Journal.h"
#else
#include "Journal.h"
#endif

#include <gmock/gmock.h>

class MockJournalHelperInterface : public Sophos::Journal::HelperInterface
{
public:
    MockJournalHelperInterface() = default;
    MOCK_METHOD(
        std::shared_ptr<Sophos::Journal::ViewInterface>,
        GetJournalView,
        (const Sophos::Journal::Subjects&, const Sophos::Journal::JRL&),
        (const, override));
    MOCK_METHOD(
        std::shared_ptr<Sophos::Journal::ViewInterface>,
        GetJournalView,
        (const Sophos::Journal::Subjects&, Sophos::Journal::JournalTime),
        (const, override));
    MOCK_METHOD(
        std::shared_ptr<Sophos::Journal::ViewInterface>,
        GetJournalView,
        (const Sophos::Journal::Subjects&, Sophos::Journal::JournalTime, Sophos::Journal::JournalTime),
        (const, override));
};

class MockImplementationInterface : public Sophos::Journal::ViewInterface::ConstIterator::ImplementationInterface
{
public:
    MockImplementationInterface() = default;

    MOCK_METHOD(reference, Dereference, (), (override));
    MOCK_METHOD(std::shared_ptr<self_type>, Copy, (), (override));
    MOCK_METHOD(self_type&, PrefixIncrement, (), (override));
    MOCK_METHOD(pointer, Address, (), (override));
    MOCK_METHOD(Sophos::Journal::JRL, GetJournalResourceLocator, (), (const, override));

    MOCK_METHOD(self_type&, Assign, ());

    #ifdef SPL_BAZEL
    MOCK_METHOD(bool, Equals, (), (const));
    bool operator==([[maybe_unused]] const self_type& rhs) const override
    {
        return Equals();
    }

    bool operator!=([[maybe_unused]] const self_type& rhs) const override
    {
        return !Equals();
    }
    #else
    MOCK_METHOD(bool, Equals, ());
    bool operator==([[maybe_unused]] const self_type& rhs) override
    {
        return Equals();
    }

    bool operator!=([[maybe_unused]] const self_type& rhs) override
    {
        return !Equals();
    }
    #endif

    self_type& operator=([[maybe_unused]] const self_type& rhs) override
    {
        return Assign();
    }
};

class MockJournalViewInterface : public Sophos::Journal::ViewInterface
{
public:
    MockJournalViewInterface() = default;

    MOCK_METHOD(ConstIterator, cbegin, (), (override));
    MOCK_METHOD(ConstIterator, cend, (), (override));
};

class MockJournalEntryInterface : public Sophos::Journal::ViewInterface::EntryInterface
{
public:
    MockJournalEntryInterface() = default;
    MOCK_METHOD(uint16_t, GetHeaderVersion, (), (const, override));
    MOCK_METHOD(Sophos::Journal::SerializationMethod, GetSerializationMethod, (), (const, override));
    MOCK_METHOD(Sophos::Journal::SerializationMethodVersion, GetSerializationMethodVersion, (), (const, override));
    MOCK_METHOD(Sophos::Journal::Producer, GetProducerName, (), (const, override));
    MOCK_METHOD(Sophos::Journal::Subject, GetSubjectName, (), (const, override));
    MOCK_METHOD(Sophos::Journal::ProducerId, GetProducerUniqueId, (), (const, override));
    MOCK_METHOD(FILETIME, GetTimestamp, (), (const, override));
    MOCK_METHOD(const std::byte*, GetData, (), (const, override));
    MOCK_METHOD(size_t, GetDataSize, (), (const, override));
    MOCK_METHOD(
        bool,
        InRange,
        (Sophos::Journal::JournalTime startTime, Sophos::Journal::JournalTime endTime),
        (const, override));
};
