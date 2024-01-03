// Copyright 2023 Sophos Limited. All rights reserved.

#include "Common/ConfigFile/ConfigFile.h"
#include "Common/FileSystem/IFileNotFoundException.h"
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

TEST_F(TestConfigFile, throwForMissingValue)
{
    ConfigFile c{{}};
    EXPECT_THROW(c.at("FOO"), std::out_of_range);
}

TEST_F(TestConfigFile, constructWithValueInVector)
{
    ConfigFile c{{"FOO=XXX"}};
    EXPECT_EQ(c.get("FOO", "BAR"), "XXX");
    EXPECT_EQ(c.at("FOO"), "XXX");
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

TEST_F(TestConfigFile, missingFile)
{
    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, readLines(_)).WillOnce(Throw(IFileNotFoundException(LOCATION, "Test exception")));
    ConfigFile c{mockFileSystem.get(), "/path"};
    EXPECT_EQ(c.get("FOO", "BAR"), "BAR");
}

TEST_F(TestConfigFile, missingFileNotIgnored)
{
    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, readLines(_)).WillOnce(Throw(IFileNotFoundException(LOCATION, "Test exception")));
    EXPECT_THROW(ConfigFile c(mockFileSystem.get(), "/path", false), IFileNotFoundException);
}

TEST_F(TestConfigFile, readFailureThrown)
{
    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, readLines(_)).WillOnce(Throw(IFileSystemException(LOCATION, "Test exception")));
    EXPECT_THROW(ConfigFile c(mockFileSystem.get(), "/path"), IFileSystemException);
}

TEST_F(TestConfigFile, stripSpaceFromStartOfValue)
{
    ConfigFile::lines_t lines;
    lines.emplace_back("FOO= BAR");
    ConfigFile c{lines};
    EXPECT_EQ(c.get("FOO", "XXX"), "BAR");
}

TEST_F(TestConfigFile, malformedIniFile)
{
    ConfigFile::lines_t lines;
    lines.emplace_back("FOO");
    ConfigFile c{lines};
    EXPECT_EQ(c.get("FOO", "XXX"), "XXX");
}

TEST_F(TestConfigFile, largeValue)
{
    ConfigFile::lines_t lines;
    std::string expectedValue(1000000, 'a');
    lines.emplace_back("FOO=" + expectedValue);
    ConfigFile c{lines};
    EXPECT_EQ(c.get("FOO", "XXX"), expectedValue);
}

TEST_F(TestConfigFile, multipleEquals)
{
    ConfigFile::lines_t lines;
    lines.emplace_back("FOO=BAR=YYY");
    ConfigFile c{lines};
    EXPECT_EQ(c.get("FOO", "XXX"), "BAR=YYY");
}

TEST_F(TestConfigFile, emptyValue)
{
    ConfigFile::lines_t lines;
    lines.emplace_back("FOO=");
    ConfigFile c{lines};
    EXPECT_EQ(c.get("FOO", "XXX"), "");
}

TEST_F(TestConfigFile, booleanTest)
{
    ConfigFile::lines_t lines;
    lines.emplace_back("A=");
    lines.emplace_back("B=true");
    lines.emplace_back("C=1");
    lines.emplace_back("D=false");
    lines.emplace_back("E = true");
    ConfigFile c{lines};
    EXPECT_FALSE(c.getBoolean("A", true));
    EXPECT_TRUE(c.getBoolean("B", false));
    EXPECT_FALSE(c.getBoolean("C", true));
    EXPECT_FALSE(c.getBoolean("D", true));
    EXPECT_TRUE(c.getBoolean("E", false));
    EXPECT_TRUE(c.getBoolean("F", true));
    EXPECT_FALSE(c.getBoolean("F", false));
}






