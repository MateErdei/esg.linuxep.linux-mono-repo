/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

/**
 * Component tests to Telemetry
 */

//#include "ConfigurationSettings.pb.h"
//#include "MockVersig.h"
//#include "MockWarehouseRepository.h"
//#include "TestWarehouseHelper.h"

//#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
//#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
//#include <Common/FileSystem/IFileSystemException.h>
//#include <Common/FileSystemImpl/FileSystemImpl.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
//#include <Common/ProcessImpl/ArgcAndEnv.h>
//#include <Common/ProcessImpl/ProcessImpl.h>
//#include <Common/UtilityImpl/MessageUtility.h>
#include <Telemetry/Telemetry.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
//#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
//#include <tests/Common/Helpers/MockFileSystem.h>
//#include <tests/Common/OSUtilitiesImpl/MockPidLockFileUtils.h>
//#include <tests/Common/ProcessImpl/MockProcess.h>

class TelemetryTest : public ::testing::Test
{
    Common::Logging::ConsoleLoggingSetup m_consoleLogging;

public:
    /**
     * Setup directories and files expected by Telemetry to enable its execution.
     * Use TempDir
     */
    void SetUp() override
    {
        Test::SetUp();
    }

    /**
     * Remove the temporary directory.
     */
    void TearDown() override
    {
        //Tests::restoreFileSystem();
        Test::TearDown();
    }
};

TEST_F(TelemetryTest, main_entry_MessageIsLogged) // NOLINT
{
    char args[] = {'M', 'E', 'L'};
    int expectedErrorCode = 0;

    EXPECT_EQ(Telemetry::main_entry(3, args, expectedErrorCode);
}