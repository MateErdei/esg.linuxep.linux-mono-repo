// Copyright 2023 Sophos Limited. All rights reserved.

#include "ApplicationConfigurationImpl/ApplicationPathManager.h"
#include "FileSystem/IFileSystemException.h"
#include "tests/Common/Helpers/LogInitializedTests.h"
#include "tests/Common/Helpers/MockFilePermissions.h"
#include "watchdog/watchdogimpl/UserGroupUtils.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace watchdog::watchdogimpl;
class TestUserGroupUtils : public LogOffInitializedTests
{
    Tests::ScopedReplaceFilePermissions m_filePermissionsReplacer;

protected:
    MockFilePermissions* m_mockFilePermissionsPtr;

public:
    TestUserGroupUtils()
    {
        m_mockFilePermissionsPtr = new NaggyMock<MockFilePermissions>();
        m_filePermissionsReplacer.replace(
            std::unique_ptr<Common::FileSystem::IFilePermissions>(m_mockFilePermissionsPtr));
    }
    ~TestUserGroupUtils() override { Tests::restoreFilePermissions(); }
};

TEST_F(TestUserGroupUtils, CommentsAreStrippedFromRequestedUserAndGroupIdsFileReturningValidJson)
{
    nlohmann::json expectedJson = {
        {"users", {{"user1", 1},{"user2", 2}}},
        {"groups", {{"group1", 1},{"group2", 2}}}
    };

    std::vector<std::string> configString{
        "#Comment here",
        "#Another comment here",
        "{",
            R"("users": {)",
                R"("user1": 1,)",
                R"("user2": 2)",
            "},",
            R"("groups": {)",
                R"("group1": 1,)",
                R"("group2": 2)",
            "}",
        "}"
    };

    auto strippedConfigString = stripCommentsFromRequestedUserGroupIdFile(configString);
    EXPECT_EQ(nlohmann::json::parse(strippedConfigString), expectedJson);
}

TEST_F(TestUserGroupUtils, DifferentStructuresOfCommentsAreStrippedFromRequestedUserAndGroupIdsFileReturningValidJson)
{
    nlohmann::json expectedJson = {
        {"users", {{"user1", 1},{"user2", 2}}},
        {"groups", {{"group1", 1},{"group2", 2}}}
    };

    std::vector<std::string> configString{
        "#Comment here",
        "#Another comment here",
        " #Comment with spacing here",
        " # Comment with more spacing here",
        "               #Comment with lots more spacing here",
        "## Comment with multiple hashes here",
        "{",
            R"("users": {)",
                R"("user1": 1,)",
                R"("user2": 2)",
            "},",
            R"("groups": {)",
                R"("group1": 1,)",
                R"("group2": 2)",
            "}",
        "}"
    };

    auto strippedConfigString = stripCommentsFromRequestedUserGroupIdFile(configString);
    EXPECT_EQ(nlohmann::json::parse(strippedConfigString), expectedJson);
}

TEST_F(TestUserGroupUtils, validateUserAndGroupIdsRemovesRoot)
{
    nlohmann::json configJson = {
        {"users", {{"user1", 1},{"root", 0},{"user2", 2}}},
        {"groups", {{"group1", 1},{"root", 0},{"group2", 2}}}
    };

    nlohmann::json validatedJson = {
        {"users", {{"user1", 1},{"user2", 2}}},
        {"groups", {{"group1", 1},{"group2", 2}}}
    };

    EXPECT_CALL(*m_mockFilePermissionsPtr, getAllGroupNamesAndIds()).WillOnce(Return(std::map<std::string, gid_t>{}));
    EXPECT_CALL(*m_mockFilePermissionsPtr, getAllUserNamesAndIds()).WillOnce(Return(std::map<std::string, uid_t>{}));

    EXPECT_EQ(validateUserAndGroupIds(configJson), validatedJson);
}

TEST_F(TestUserGroupUtils, validateUserAndGroupIdsRemovesDuplicateIds)
{
    nlohmann::json configJson = {
        {"users", {{"user1", 1},{"root", 0},{"user2", 2}}},
        {"groups", {{"group1", 1},{"root", 0},{"group2", 2}}}
    };

    nlohmann::json validatedJson = {
        {"users", {{"user2", 2}}},
        {"groups", {{"group2", 2}}}
    };

    EXPECT_CALL(*m_mockFilePermissionsPtr, getAllGroupNamesAndIds()).WillOnce(Return(std::map<std::string, gid_t>{{"group1", 1}}));
    EXPECT_CALL(*m_mockFilePermissionsPtr, getAllUserNamesAndIds()).WillOnce(Return(std::map<std::string, uid_t>{{"user1", 1}}));

    EXPECT_EQ(validateUserAndGroupIds(configJson), validatedJson);
}

TEST_F(TestUserGroupUtils, validateUserAndGroupIdsReturnsEmptyWhenGroupDatabasesCannotBeRead)
{
    nlohmann::json configJson = {
        {"users", {{"user1", 1},{"user2", 2}}},
        {"groups", {{"group1", 1},{"group2", 2}}}
    };

    EXPECT_CALL(*m_mockFilePermissionsPtr, getAllGroupNamesAndIds())
        .WillOnce(Throw(Common::FileSystem::IFileSystemException("Failed")));

    ASSERT_TRUE(validateUserAndGroupIds(configJson).empty());
}

TEST_F(TestUserGroupUtils, validateUserAndGroupIdsReturnsEmptyWhenUserDatabaseCannotBeRead)
{
    nlohmann::json configJson = {
        {"users", {{"user1", 1},{"user2", 2}}},
        {"groups", {{"group1", 1},{"group2", 2}}}
    };

    EXPECT_CALL(*m_mockFilePermissionsPtr, getAllUserNamesAndIds())
        .WillOnce(Throw(Common::FileSystem::IFileSystemException("Failed")));

    ASSERT_TRUE(validateUserAndGroupIds(configJson).empty());
}

TEST_F(TestUserGroupUtils, validateUserAndGroupIdsHandlesOnlyGroups)
{
    nlohmann::json configJson = {
        {"groups", {{"group1", 1},{"group2", 2}}}
    };

    EXPECT_CALL(*m_mockFilePermissionsPtr, getAllGroupNamesAndIds()).WillOnce(Return(std::map<std::string, gid_t>{}));
    EXPECT_CALL(*m_mockFilePermissionsPtr, getAllUserNamesAndIds()).WillOnce(Return(std::map<std::string, uid_t>{}));

    EXPECT_EQ(validateUserAndGroupIds(configJson), configJson);
}

TEST_F(TestUserGroupUtils, validateUserAndGroupIdsRemovesDuplicateIdsWhenThereAreOnlyGroups)
{
    nlohmann::json configJson = {
        {"groups", {{"group1", 1},{"group2", 2}}}
    };

    nlohmann::json validatedJson = {
        {"groups", {{"group2", 2}}}
    };

    EXPECT_CALL(*m_mockFilePermissionsPtr, getAllGroupNamesAndIds()).WillOnce(Return(std::map<std::string, gid_t>{{"group1", 1}}));
    EXPECT_CALL(*m_mockFilePermissionsPtr, getAllUserNamesAndIds()).WillOnce(Return(std::map<std::string, uid_t>{{"user1", 1}}));

    EXPECT_EQ(validateUserAndGroupIds(configJson), validatedJson);
}

TEST_F(TestUserGroupUtils, validateUserAndGroupIdsHandlesOnlyUsers)
{
    nlohmann::json configJson = {
        {"users", {{"user1", 1},{"user2", 2}}}
    };

    EXPECT_CALL(*m_mockFilePermissionsPtr, getAllGroupNamesAndIds()).WillOnce(Return(std::map<std::string, gid_t>{}));
    EXPECT_CALL(*m_mockFilePermissionsPtr, getAllUserNamesAndIds()).WillOnce(Return(std::map<std::string, uid_t>{}));

    EXPECT_EQ(validateUserAndGroupIds(configJson), configJson);
}

TEST_F(TestUserGroupUtils, validateUserAndGroupIdsRemovesDuplicateIdsWhenThereAreOnlyUsers)
{
    nlohmann::json configJson = {
        {"users", {{"user1", 1},{"user2", 2}}}
    };

    nlohmann::json validatedJson = {
        {"users", {{"user2", 2}}}
    };

    EXPECT_CALL(*m_mockFilePermissionsPtr, getAllGroupNamesAndIds()).WillOnce(Return(std::map<std::string, gid_t>{{"group1", 1}}));
    EXPECT_CALL(*m_mockFilePermissionsPtr, getAllUserNamesAndIds()).WillOnce(Return(std::map<std::string, uid_t>{{"user1", 1}}));

    EXPECT_EQ(validateUserAndGroupIds(configJson), validatedJson);
}
