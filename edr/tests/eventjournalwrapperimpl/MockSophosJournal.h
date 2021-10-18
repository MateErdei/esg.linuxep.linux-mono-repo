/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include <modules/EventJournalWrapperImpl/EventJournalTimeUtils.h>
#include <Journal.h>
#include <gmock/gmock.h>


class MockJournalHelperInterface : public Sophos::Journal::HelperInterface
{
public:
    MockJournalHelperInterface() = default;
    MOCK_CONST_METHOD2(GetJournalView, std::shared_ptr<Sophos::Journal::ViewInterface>(const Sophos::Journal::Subjects&, const Sophos::Journal::JRL&));
    MOCK_CONST_METHOD2(GetJournalView, std::shared_ptr<Sophos::Journal::ViewInterface>(const Sophos::Journal::Subjects&, Sophos::Journal::JournalTime));
    MOCK_CONST_METHOD3(GetJournalView, std::shared_ptr<Sophos::Journal::ViewInterface>(const Sophos::Journal::Subjects&, Sophos::Journal::JournalTime, Sophos::Journal::JournalTime));
};
