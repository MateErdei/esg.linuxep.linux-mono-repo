/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <log4cplus/logger.h>
#include <Common/Logging/LoggerConfig.h>
#include <Common/Logging/ConsoleLoggingSetup.h>

namespace test_common
{
    void initialize_logging();
}

namespace
{
    void initialize_logging()
    {
        test_common::initialize_logging();
    }

    /** Inherit from this class when the tests 'uses' log4cplus and the messages are used in the tests*/
    class LogInitializedTests : public ::testing::Test
    {
    public:
        LogInitializedTests()
        {
            initialize_logging();
        }
    };

    class SetupTestLogging
    {
    public:
        SetupTestLogging()
        {
            initialize_logging();
        }
    };

}