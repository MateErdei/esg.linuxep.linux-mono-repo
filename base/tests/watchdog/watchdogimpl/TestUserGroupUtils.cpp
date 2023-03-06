// Copyright 2023 Sophos Limited. All rights reserved.

#include "ApplicationConfigurationImpl/ApplicationPathManager.h"
#include "FileSystem/IFileSystemException.h"
#include "ProcessImpl/ProcessImpl.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/LogInitializedTests.h"
#include "tests/Common/Helpers/MockFilePermissions.h"
#include "tests/Common/Helpers/MockFileSystem.h"
#include "tests/Common/Helpers/MockProcess.h"
#include "tests/Common/Helpers/TempDir.h"
#include "watchdog/watchdogimpl/UserGroupUtils.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace watchdog::watchdogimpl;
class TestUserGroupUtilsRealFileSystem: public LogOffInitializedTests{};

class TestUserGroupUtils : public LogOffInitializedTests
{
    Tests::ScopedReplaceFileSystem m_fileSystemReplacer;
    Tests::ScopedReplaceFilePermissions m_filePermissionsReplacer;

protected:
    MockFileSystem* m_mockFileSystemPtr;
    MockFilePermissions* m_mockFilePermissionsPtr;
    std::string m_requestedUserGroupIdConfigPath;

public:
    TestUserGroupUtils()
    {
        m_mockFileSystemPtr = new NaggyMock<MockFileSystem>();
        m_mockFilePermissionsPtr = new NaggyMock<MockFilePermissions>();

        m_fileSystemReplacer.replace(std::unique_ptr<Common::FileSystem::IFileSystem>(m_mockFileSystemPtr));
        m_filePermissionsReplacer.replace(std::unique_ptr<Common::FileSystem::IFilePermissions>(m_mockFilePermissionsPtr));
        m_requestedUserGroupIdConfigPath = Common::ApplicationConfiguration::applicationPathManager().getRequestedUserGroupIdConfigPath();
    }
    ~TestUserGroupUtils() override { Tests::restoreFilePermissions(); }

    void mockExecUserOrGroupIdChange(const std::string& executablePath, const std::string& userOrGroupSpecifier, const std::string& newId, const std::string& name)
    {
        EXPECT_CALL(*m_mockFileSystemPtr, isExecutable(executablePath)).WillRepeatedly(Return(true));
        Common::ProcessImpl::ProcessFactory::instance().replaceCreator([executablePath, userOrGroupSpecifier, newId, name]() {
            auto mockProcess = new StrictMock<MockProcess>();
            EXPECT_CALL(*mockProcess, exec(executablePath, std::vector<std::string>{ userOrGroupSpecifier, newId, name })).Times(1);
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            EXPECT_CALL(*mockProcess, wait(Common::Process::milli(100), 100)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        });
    }
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

    EXPECT_CALL(*m_mockFileSystemPtr, readLines(m_requestedUserGroupIdConfigPath)).WillOnce(Return(configString));
    EXPECT_EQ(readRequestedUserGroupIds(), expectedJson);
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

    EXPECT_CALL(*m_mockFileSystemPtr, readLines(m_requestedUserGroupIdConfigPath)).WillOnce(Return(configString));
    EXPECT_EQ(readRequestedUserGroupIds(), expectedJson);
}

TEST_F(TestUserGroupUtils, readRequestedUserGroupIdsDoesNotThrowOnInvalidJson)
{
    WatchdogUserGroupIDs changesNeeded;
    std::vector<std::string> configString{
        "Plain text here",
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

    EXPECT_CALL(*m_mockFileSystemPtr, readLines(m_requestedUserGroupIdConfigPath)).WillOnce(Return(configString));

    EXPECT_NO_THROW(changesNeeded = readRequestedUserGroupIds());
    ASSERT_TRUE(changesNeeded.empty());
}

TEST_F(TestUserGroupUtils, readRequestedUserGroupIdsDoesNotThrowOnCorrectlyStrippedInvalidJson)
{
    WatchdogUserGroupIDs changesNeeded;
    std::vector<std::string> configString{
        "{",
            R"("users": {)",
                R"("user1": 1,)",
                R"("user2": 2)",
            R"("groups": {)",
                R"("group1": 1,)",
                R"("group2": 2)",
            "}",
        "}"
    };

    EXPECT_CALL(*m_mockFileSystemPtr, readLines(m_requestedUserGroupIdConfigPath)).WillOnce(Return(configString));

    EXPECT_NO_THROW(changesNeeded = readRequestedUserGroupIds());
    ASSERT_TRUE(changesNeeded.empty());
}

TEST_F(TestUserGroupUtils, readRequestedUserGroupIdsDoesNotThrowWhenReadLinesThrows)
{
    WatchdogUserGroupIDs changesNeeded;

    EXPECT_CALL(*m_mockFileSystemPtr, readLines(m_requestedUserGroupIdConfigPath)).WillOnce(Throw(Common::FileSystem::IFileSystemException("TEST")));

    EXPECT_NO_THROW(changesNeeded = readRequestedUserGroupIds());
    ASSERT_TRUE(changesNeeded.empty());
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

TEST_F(TestUserGroupUtils, changeUserId)
{
    mockExecUserOrGroupIdChange("/usr/sbin/usermod", "-u", "999", "user");
    EXPECT_NO_THROW(changeUserId("user", 999));
}

TEST_F(TestUserGroupUtils, changeUserIdThrowsWhenExitCodeIsNotZero)
{
    EXPECT_CALL(*m_mockFileSystemPtr, isExecutable("/usr/sbin/usermod")).WillRepeatedly(Return(true));
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([]() {
        auto mockProcess = new StrictMock<MockProcess>();
        std::vector<std::string> args = { "-u", "999", "user" };
        EXPECT_CALL(*mockProcess, exec("/usr/sbin/usermod", args)).Times(1);
        EXPECT_CALL(*mockProcess, wait(Common::Process::milli(100), 100)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
        EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(1));
        EXPECT_CALL(*mockProcess, output()).WillOnce(Return(""));
        return std::unique_ptr<Common::Process::IProcess>(mockProcess);
    });

    EXPECT_THROW(changeUserId("user", 999), std::runtime_error);
}

TEST_F(TestUserGroupUtils, changeUserIdThrowsWhenCommandIsStillRunningAfterWait)
{
    EXPECT_CALL(*m_mockFileSystemPtr, isExecutable("/usr/sbin/usermod")).WillRepeatedly(Return(true));
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([]() {
        auto mockProcess = new StrictMock<MockProcess>();
        std::vector<std::string> args = { "-u", "999", "user" };
        EXPECT_CALL(*mockProcess, exec("/usr/sbin/usermod", args)).Times(1);
        EXPECT_CALL(*mockProcess, wait(Common::Process::milli(100), 100)).WillOnce(Return(Common::Process::ProcessStatus::RUNNING));
        EXPECT_CALL(*mockProcess, kill()).Times(1);
        EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(1));
        EXPECT_CALL(*mockProcess, output()).WillOnce(Return(""));
        return std::unique_ptr<Common::Process::IProcess>(mockProcess);
    });

    EXPECT_THROW(changeUserId("user", 999), std::runtime_error);
}

TEST_F(TestUserGroupUtils, changeGroupId)
{
    mockExecUserOrGroupIdChange("/usr/sbin/groupmod", "-g", "998", "group");
    EXPECT_NO_THROW(changeGroupId("group", 998));
}

TEST_F(TestUserGroupUtils, changeGroupIdThrowsWhenExitCodeIsNotZero)
{
    EXPECT_CALL(*m_mockFileSystemPtr, isExecutable("/usr/sbin/groupmod")).WillRepeatedly(Return(true));
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([]() {
        auto mockProcess = new StrictMock<MockProcess>();
        std::vector<std::string> args = { "-g", "998", "group" };
        EXPECT_CALL(*mockProcess, exec("/usr/sbin/groupmod", args)).Times(1);
        EXPECT_CALL(*mockProcess, wait(Common::Process::milli(100), 100)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
        EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(1));
        EXPECT_CALL(*mockProcess, output()).WillOnce(Return(""));
        return std::unique_ptr<Common::Process::IProcess>(mockProcess);
    });

    EXPECT_THROW(changeGroupId("group", 998), std::runtime_error);
}

TEST_F(TestUserGroupUtils, changeGroupIdThrowsWhenCommandIsStillRunningAfterWait)
{
    EXPECT_CALL(*m_mockFileSystemPtr, isExecutable("/usr/sbin/groupmod")).WillRepeatedly(Return(true));
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([]() {
        auto mockProcess = new StrictMock<MockProcess>();
        std::vector<std::string> args = { "-g", "998", "group" };
        EXPECT_CALL(*mockProcess, exec("/usr/sbin/groupmod", args)).Times(1);
        EXPECT_CALL(*mockProcess, wait(Common::Process::milli(100), 100)).WillOnce(Return(Common::Process::ProcessStatus::RUNNING));
        EXPECT_CALL(*mockProcess, kill()).Times(1);
        EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(1));
        EXPECT_CALL(*mockProcess, output()).WillOnce(Return(""));
        return std::unique_ptr<Common::Process::IProcess>(mockProcess);
    });

    EXPECT_THROW(changeGroupId("group", 998), std::runtime_error);
}

TEST_F(TestUserGroupUtilsRealFileSystem, remapUserIdOfFiles)
{
    auto filePermissions = Common::FileSystem::filePermissions();
    std::unique_ptr<Tests::TempDir> tempDir = Tests::TempDir::makeTempDir();
    
    std::string startingDir = tempDir->dirPath();
    tempDir->makeDirs("directory1");
    tempDir->makeDirs("directory2");
    tempDir->makeDirs("directory2/subdirectory");
    tempDir->makeDirs("directory3");
    tempDir->createFile("file1", "contents");
    tempDir->createFile("file2", "contents");
    tempDir->createFile("directory2/file1", "contents");
    tempDir->createFile("directory2/subdirectory/file2", "contents");
    tempDir->createFile("directory3/file1", "contents");
    tempDir->createFile("directory3/file2", "contents");
    tempDir->createFile("directory3/file3", "contents");
    
    EXPECT_NO_THROW(remapUserIdOfFiles(startingDir, filePermissions->getUserIdOfDirEntry(startingDir), 2));

    EXPECT_EQ(filePermissions->getUserIdOfDirEntry(Common::FileSystem::join(startingDir,"directory1")), 2);
    EXPECT_EQ(filePermissions->getUserIdOfDirEntry(Common::FileSystem::join(startingDir,"directory2")), 2);
    EXPECT_EQ(filePermissions->getUserIdOfDirEntry(Common::FileSystem::join(startingDir,"directory2/subdirectory")), 2);
    EXPECT_EQ(filePermissions->getUserIdOfDirEntry(Common::FileSystem::join(startingDir,"directory3")), 2);
    EXPECT_EQ(filePermissions->getUserIdOfDirEntry(Common::FileSystem::join(startingDir,"file1")), 2);
    EXPECT_EQ(filePermissions->getUserIdOfDirEntry(Common::FileSystem::join(startingDir,"file2")), 2);
    EXPECT_EQ(filePermissions->getUserIdOfDirEntry(Common::FileSystem::join(startingDir,"directory2/file1")), 2);
    EXPECT_EQ(filePermissions->getUserIdOfDirEntry(Common::FileSystem::join(startingDir,"directory2/subdirectory/file2")), 2);
    EXPECT_EQ(filePermissions->getUserIdOfDirEntry(Common::FileSystem::join(startingDir,"directory3/file1")), 2);
    EXPECT_EQ(filePermissions->getUserIdOfDirEntry(Common::FileSystem::join(startingDir,"directory3/file2")), 2);
    EXPECT_EQ(filePermissions->getUserIdOfDirEntry(Common::FileSystem::join(startingDir,"directory3/file3")), 2);
}

TEST_F(TestUserGroupUtilsRealFileSystem, remapGroupIdOfFiles)
{
    auto filePermissions = Common::FileSystem::filePermissions();
    std::unique_ptr<Tests::TempDir> tempDir = Tests::TempDir::makeTempDir();

    std::string startingDir = tempDir->dirPath();
    tempDir->makeDirs("directory1");
    tempDir->makeDirs("directory2");
    tempDir->makeDirs("directory2/subdirectory");
    tempDir->makeDirs("directory3");
    tempDir->createFile("file1", "contents");
    tempDir->createFile("file2", "contents");
    tempDir->createFile("directory2/file1", "contents");
    tempDir->createFile("directory2/subdirectory/file2", "contents");
    tempDir->createFile("directory3/file1", "contents");
    tempDir->createFile("directory3/file2", "contents");
    tempDir->createFile("directory3/file3", "contents");

    EXPECT_NO_THROW(remapGroupIdOfFiles(startingDir, filePermissions->getGroupIdOfDirEntry(startingDir), 2));

    EXPECT_EQ(filePermissions->getGroupIdOfDirEntry(Common::FileSystem::join(startingDir,"directory1")), 2);
    EXPECT_EQ(filePermissions->getGroupIdOfDirEntry(Common::FileSystem::join(startingDir,"directory2")), 2);
    EXPECT_EQ(filePermissions->getGroupIdOfDirEntry(Common::FileSystem::join(startingDir,"directory2/subdirectory")), 2);
    EXPECT_EQ(filePermissions->getGroupIdOfDirEntry(Common::FileSystem::join(startingDir,"directory3")), 2);
    EXPECT_EQ(filePermissions->getGroupIdOfDirEntry(Common::FileSystem::join(startingDir,"file1")), 2);
    EXPECT_EQ(filePermissions->getGroupIdOfDirEntry(Common::FileSystem::join(startingDir,"file2")), 2);
    EXPECT_EQ(filePermissions->getGroupIdOfDirEntry(Common::FileSystem::join(startingDir,"directory2/file1")), 2);
    EXPECT_EQ(filePermissions->getGroupIdOfDirEntry(Common::FileSystem::join(startingDir,"directory2/subdirectory/file2")), 2);
    EXPECT_EQ(filePermissions->getGroupIdOfDirEntry(Common::FileSystem::join(startingDir,"directory3/file1")), 2);
    EXPECT_EQ(filePermissions->getGroupIdOfDirEntry(Common::FileSystem::join(startingDir,"directory3/file2")), 2);
    EXPECT_EQ(filePermissions->getGroupIdOfDirEntry(Common::FileSystem::join(startingDir,"directory3/file3")), 2);
}
