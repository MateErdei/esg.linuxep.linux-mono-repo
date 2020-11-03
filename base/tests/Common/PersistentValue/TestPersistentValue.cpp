/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <Common/PersistentValue/PersistentValue.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/MockFileSystem.h>

class TestPersistentValue:  public ::testing::Test {};

TEST_F(TestPersistentValue, persistentValueDeafaultsIfNoFilePresent) // NOLINT
{
    std::string pathToVarDir = "var";
    std::string valueName = "aPersistedValue";
    std::string path = Common::FileSystem::join(pathToVarDir, "persist-" + valueName);
    int defaultValue = 10;
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });
    EXPECT_CALL(*mockFileSystem, exists(path)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, writeFile(path, std::to_string(defaultValue))).Times(1);
    Common::PersistentValue<int> value(pathToVarDir,valueName, defaultValue);
    auto readValue = value.getValue();
    ASSERT_EQ(readValue, defaultValue);
}

TEST_F(TestPersistentValue, persistentValueLoadsFromFile) // NOLINT
{
    std::string pathToVarDir = "var";
    std::string valueName = "aPersistedValue";
    std::string path = Common::FileSystem::join(pathToVarDir, "persist-" + valueName);
    int defaultValue = 10;
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });
    EXPECT_CALL(*mockFileSystem, exists(path)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, readFile(path)).WillOnce(Return("123"));
    EXPECT_CALL(*mockFileSystem, writeFile(path, "123")).Times(1);
    Common::PersistentValue<int> value(pathToVarDir,valueName, defaultValue);
    auto readValue = value.getValue();
    ASSERT_EQ(readValue, 123);
}

TEST_F(TestPersistentValue, setIntPersistentValue) // NOLINT
{
    std::string pathToVarDir = "var";
    std::string valueName = "aPersistedValue";
    std::string path = Common::FileSystem::join(pathToVarDir, "persist-" + valueName);
    int defaultValue = 10;
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });
    EXPECT_CALL(*mockFileSystem, exists(path)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, readFile(path)).WillOnce(Return("123"));
    EXPECT_CALL(*mockFileSystem, writeFile(path, "14")).Times(1);
    Common::PersistentValue<int> value(pathToVarDir,valueName, defaultValue);
    value.setValue(14);
    ASSERT_EQ(value.getValue(), 14);
}

TEST_F(TestPersistentValue, setUnsignedIntPersistentValue) // NOLINT
{
    std::string pathToVarDir = "var";
    std::string valueName = "aPersistedValue";
    std::string path = Common::FileSystem::join(pathToVarDir, "persist-" + valueName);
    unsigned int defaultValue = 10;
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });
    EXPECT_CALL(*mockFileSystem, exists(path)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, readFile(path)).WillOnce(Return("123"));
    EXPECT_CALL(*mockFileSystem, writeFile(path, "14")).Times(1);
    Common::PersistentValue<unsigned int> value(pathToVarDir,valueName, defaultValue);
    value.setValue(14);
    ASSERT_EQ(value.getValue(), 14);
}

TEST_F(TestPersistentValue, setFloatPersistentValue) // NOLINT
{
    std::string pathToVarDir = "var";
    std::string valueName = "aPersistedValue";
    std::string path = Common::FileSystem::join(pathToVarDir, "persist-" + valueName);
    float defaultValue = 10.2;
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });
    EXPECT_CALL(*mockFileSystem, exists(path)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, readFile(path)).WillOnce(Return("14.2"));
    EXPECT_CALL(*mockFileSystem, writeFile(path, "14.2")).Times(1);
    Common::PersistentValue<float> value(pathToVarDir,valueName, defaultValue);
    value.setValue(14.2);
    float difference = abs(value.getValue() - 14.2);
    float epsilon = 0.0001;
    ASSERT_TRUE(difference < epsilon);
}

TEST_F(TestPersistentValue, setStringPersistentValue) // NOLINT
{
    std::string pathToVarDir = "var";
    std::string valueName = "aPersistedValue";
    std::string path = Common::FileSystem::join(pathToVarDir, "persist-" + valueName);
    std::string defaultValue = "someValue";
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });
    EXPECT_CALL(*mockFileSystem, exists(path)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, readFile(path)).WillOnce(Return("some value already there"));
    EXPECT_CALL(*mockFileSystem, writeFile(path, "another value")).Times(1);
    Common::PersistentValue<std::string> value(pathToVarDir,valueName, defaultValue);
    value.setValue("another value");
    ASSERT_EQ(value.getValue(), "another value");
}
