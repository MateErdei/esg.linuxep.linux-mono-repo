// Copyright 2022-2023 Sophos Limited. All rights reserved.
#include "cmcsrouter/AgentAdapter.h"
#include <gtest/gtest.h>
#include "cmcsrouter/ConfigOptions.h"
#include "tests/Common/Helpers/MockFileSystem.h"
#include "tests/Common/Helpers/MockPlatformUtils.h"

#include <sstream>
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"


TEST(TestConfigOptions, testConfigOptionsWrittenCorrectly)
{
    auto *mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));

    EXPECT_CALL(*mockFileSystem, writeFile("pathToWriteTo", "KEY1=1\nKEY2=2\nKEY3=3\n"));

    MCS::ConfigOptions testConfig;
    testConfig.config["KEY1"] = "1";
    testConfig.config["KEY2"] = "2";
    testConfig.config["KEY3"] = "3";
    testConfig.writeToDisk("pathToWriteTo");

    EXPECT_CALL(*mockFileSystem, writeFile("secondPathToWriteTo", "KEY_A=Value1\nKEY_B=Value2\nKEY_C=Value3\n"));

    MCS::ConfigOptions testConfig2;
    testConfig2.config["KEY_A"] = "Value1";
    testConfig2.config["KEY_B"] = "Value2";
    testConfig2.config["KEY_C"] = "Value3";
    testConfig2.writeToDisk("secondPathToWriteTo");
}
