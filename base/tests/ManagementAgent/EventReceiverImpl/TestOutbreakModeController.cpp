// Copyright 2023 Sophos Limited. All rights reserved.

#include "tests/Common/Helpers/MemoryAppender.h"

#include "ManagementAgent/EventReceiverImpl/OutbreakModeController.h"

using namespace ManagementAgent::EventReceiverImpl;

namespace
{
    class TestOutbreakModeController : public MemoryAppenderUsingTests
    {
    public:
        TestOutbreakModeController()
            : MemoryAppenderUsingTests("")
        {}
    };
}

TEST_F(TestOutbreakModeController, construction)
{
    EXPECT_NO_THROW(std::make_shared<OutbreakModeController>());
}
