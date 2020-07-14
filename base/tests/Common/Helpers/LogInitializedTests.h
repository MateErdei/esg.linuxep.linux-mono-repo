/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <log4cplus/logger.h>
#include <Common/Logging/LoggerConfig.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
/** Inherit from this class when the tests 'uses' log4cplus and the messages are used in the tests*/
class LogInitializedTests : public ::testing::Test
{    
    Common::Logging::ConsoleLoggingSetup m_loggingSetup;
};

/** Inherit from this class when the tests 'uses' log4cplus but the log messages are irrelevant*/
class LogOffInitializedTests: public ::testing::Test
{
    Common::Logging::ConsoleLoggingSetup m_loggingSetup;
    public:
    LogOffInitializedTests():m_loggingSetup{Common::Logging::LOGOFFFORTEST()}{
    }
};
