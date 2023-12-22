// Copyright 2023 Sophos Limited. All rights reserved.

#include "Common/ConfigFile/ConfigFile.h"
#include "Common/FileSystem/IFileSystem.h"

#include "tests/Common/Helpers/LogInitializedTests.h"
#include "tests/Common/Helpers/MockFileSystem.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace
{
    class TestConfigFile : public LogInitializedTests
    {
    };
}

using namespace Common::ConfigFile;

TEST_F(TestConfigFile, emptyVectorConstruction)
{
    ConfigFile c{{}};
    EXPECT_EQ(c.get("FOO", "BAR"), "BAR");
}


TEST_F(TestConfigFile, constructWithValueInVector)
{
    ConfigFile c{{"FOO=XXX"}};
    EXPECT_EQ(c.get("FOO", "BAR"), "XXX");
}

TEST_F(TestConfigFile, constructWithValueInVectorWithSpaces)
{
    ConfigFile c{{" FOO = XXX"}};
    EXPECT_EQ(c.get("FOO", "BAR"), "XXX");
}

TEST_F(TestConfigFile, constructWithRepeat)
{
    ConfigFile c{{"FOO=XXX", "FOO=BAR"}};
    EXPECT_EQ(c.get("FOO", "YYY"), "BAR");
}

TEST_F(TestConfigFile, constructWithEmptyFile)
{
    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    ConfigFile::lines_t lines;
    EXPECT_CALL(*mockFileSystem, readLines(_)).WillOnce(Return(lines));
    ConfigFile c{mockFileSystem.get(), "/path"};
    EXPECT_EQ(c.get("FOO", "BAR"), "BAR");
}

TEST_F(TestConfigFile, constructWithLineFromFile)
{
    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    ConfigFile::lines_t lines;
    lines.emplace_back("FOO=BAR");
    EXPECT_CALL(*mockFileSystem, readLines(_)).WillOnce(Return(lines));
    ConfigFile c{mockFileSystem.get(), "/path"};
    EXPECT_EQ(c.get("FOO", "XXX"), "BAR");
}


TEST_F(TestConfigFile, stripSpaceFromStartOfValue)
{
    ConfigFile::lines_t lines;
    lines.emplace_back("FOO= BAR");
    ConfigFile c{lines};
    EXPECT_EQ(c.get("FOO", "XXX"), "BAR");
}





