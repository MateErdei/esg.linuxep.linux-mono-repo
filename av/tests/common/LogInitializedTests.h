/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <log4cplus/logger.h>
#include <Common/Logging/LoggerConfig.h>
#include <Common/Logging/ConsoleLoggingSetup.h>

namespace
{
    void initialize_logging()
    {
        static Common::Logging::ConsoleLoggingSetup m_loggingSetup;
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