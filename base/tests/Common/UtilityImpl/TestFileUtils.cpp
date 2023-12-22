// Copyright 2023 Sophos Limited. All rights reserved.

#include "Common/UtilityImpl/FileUtils.h"

#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/LogInitializedTests.h"
#include "tests/Common/Helpers/MockFileSystem.h"

#include <gtest/gtest.h>

using namespace Common::UtilityImpl;

class TestFileUtils : public LogOffInitializedTests
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

TEST_F(TestFileUtils, extractValueFromFile_fileDoesNotExist)
{
    std::string filePath = "/tmp/test.ini";
    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    
    EXPECT_CALL(*mockFileSystem, isFile(filePath)).WillOnce(Return(false));
    Tests::ScopedReplaceFileSystem replaceFileSystem{std::move(mockFileSystem)};
    
    auto values = FileUtils::extractValueFromFile(filePath, "testKey");
    EXPECT_EQ(values.first, "");
    EXPECT_EQ(values.second, "File: " + filePath + " does not exist");
}