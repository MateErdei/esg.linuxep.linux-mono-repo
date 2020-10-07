#include <gtest/gtest.h>

#include "avscanner/avscannerimpl/Logger.h"
#include <log4cplus/logger.h>

TEST(TestLogger, TestConstructor) // NOLINT
{
    EXPECT_NO_THROW(Logger("/tmp/test_logger.txt", log4cplus::INFO_LOG_LEVEL, true));
    EXPECT_NO_THROW(Logger("/tmp/test_logger.txt"));
}

TEST(TestLogger, TestApplyCommandLineLevel) // NOLINT
{
    Logger log("/tmp/test_logger.txt", true);
    EXPECT_NO_THROW(log.applyCommandLineLevel(log4cplus::DEBUG_LOG_LEVEL));
}
