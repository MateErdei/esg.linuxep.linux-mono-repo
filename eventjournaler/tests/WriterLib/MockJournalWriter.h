/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
//#include "modules/WriterLib/Writer.h"

#include <gmock/gmock.h>

using namespace ::testing;
using namespace WriterLib;

class MockJournalWriter // : public WriterLib::IWriter
{
public:
    MockEventQueuePusher() = default;
    MOCK_METHOD1(write, void(Common::ZeroMQWrapper::data_t));
};
