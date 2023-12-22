// Copyright 2023 Sophos All rights reserved.

#include "Common/UpdateUtilities/CommsDataUtil.h"

#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/LogInitializedTests.h"
#include "tests/Common/Helpers/MockFileSystem.h"

#include <gtest/gtest.h>

using namespace Common::UpdateUtilities;

class TestCommsDataUtils : public LogOffInitializedTests
{
public:
    void SetUp() override
    {
        setenv("SOPHOS_TEMP_DIRECTORY", "/tmp", 1);
    }

    void TearDown() override
    {
        unsetenv("SOPHOS_TEMP_DIRECTORY");
    }
};

TEST_F(TestCommsDataUtils, writeCommsTelemetryIniFile)
{
std::string filePath1 = "/tmp/test1.ini";
std::string filePath2 = "/tmp/test2.ini";
auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();

EXPECT_CALL(*mockFileSystem, writeFile(filePath1, "usedProxy = false\nusedUpdateCache = true\nusedMessageRelay = false\nproxyOrMessageRelayURL = \n"));
EXPECT_CALL(*mockFileSystem, writeFile(filePath2, "usedProxy = true\nusedUpdateCache = false\nusedMessageRelay = true\nproxyOrMessageRelayURL = testMessageRelay\n"));
Tests::ScopedReplaceFileSystem replaceFileSystem{std::move(mockFileSystem)};

CommsDataUtil::writeCommsTelemetryIniFile(filePath1, false, true, false);
CommsDataUtil::writeCommsTelemetryIniFile(filePath2, true, false, true, "testMessageRelay");
}
