// Copyright 2023 Sophos Limited. All rights reserved.

#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/LogInitializedTests.h"
#include "tests/Common/Helpers/MockFileSystem.h"
#include "watchdog/watchdogimpl/UserGroupUtils.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

class TestUserGroupUtils : public LogOffInitializedTests{};

TEST_F(TestUserGroupUtils, Construction) // NOLINT
{
    std::vector<std::string> fileContents{
        "#Comment here",
        "#Another comment here",
        "{",
            R"("users": {)",
                R"("username1": 1,)",
                R"("username2": 2)",
            "},",
            R"("groups": {)",
                R"("group1": 1,)",
                R"("group2": 2)",
            "}",
        "}"
    };
    
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock, readLines(_)).WillOnce(Return(fileContents));
    auto mock = std::make_unique<Tests::ScopedReplaceFileSystem>(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    auto returnedValue = watchdog::watchdogimpl::readRequestedUserGroupIds();
    std::cout << returnedValue.dump();
}