// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "modules/Common/FileSystem/IPermissionDeniedException.h"
#include "modules/Common/PersistentValue/PersistentValue.h"

#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/MockFileSystem.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

class TestPersistentValue:  public ::testing::Test {};

TEST_F(TestPersistentValue, persistentValueDefaultsIfNoFilePresent)
{
    std::string pathToVarDir = "var";
    std::string valueName = "aPersistedValue";
    std::string path = Common::FileSystem::join(pathToVarDir, "persist-" + valueName);
    int defaultValue = 10;
    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, exists(path)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, writeFile(path, std::to_string(defaultValue))).Times(1);
    Tests::replaceFileSystem(std::move(mockFileSystem));
    Common::PersistentValue<int> value(pathToVarDir,valueName, defaultValue);
    auto errorValue = value.getError();
    EXPECT_EQ(errorValue, "");
    auto readValue = value.getValue();
    ASSERT_EQ(readValue, defaultValue);
}

TEST_F(TestPersistentValue, persistentValueErrorStringsDefaultsToEmpty)
{
    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, exists(_)).Times(1);
    EXPECT_CALL(*mockFileSystem, writeFile(_, _)).Times(1);
    Tests::replaceFileSystem(std::move(mockFileSystem));
    Common::PersistentValue<std::string> value("", "", "");
    auto errorValue = value.getError();
    ASSERT_EQ(errorValue, "");
}

TEST_F(TestPersistentValue, persistentValueErrorStringsIsEmptyIfFileDoesntExist)
{
    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, exists(_)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, writeFile(_, _)).Times(1);
    Tests::replaceFileSystem(std::move(mockFileSystem));
    Common::PersistentValue<std::string> value("", "", "");
    auto errorValue = value.getError();
    ASSERT_EQ(errorValue, "");
}

TEST_F(TestPersistentValue, persistentValueLoadsFromFile)
{
    std::string pathToVarDir = "var";
    std::string valueName = "aPersistedValue";
    std::string path = Common::FileSystem::join(pathToVarDir, "persist-" + valueName);
    int defaultValue = 10;
    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, exists(path)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, readFile(path)).WillOnce(Return("123"));
    EXPECT_CALL(*mockFileSystem, writeFile(path, "123")).Times(1);
    Tests::replaceFileSystem(std::move(mockFileSystem));
    Common::PersistentValue<int> value(pathToVarDir,valueName, defaultValue);
    auto readValue = value.getValue();
    auto errorValue = value.getError();
    EXPECT_EQ(errorValue, "");
    ASSERT_EQ(readValue, 123);
}

TEST_F(TestPersistentValue, setIntPersistentValue)
{
    std::string pathToVarDir = "var";
    std::string valueName = "aPersistedValue";
    std::string path = Common::FileSystem::join(pathToVarDir, "persist-" + valueName);
    int defaultValue = 10;
    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, exists(path)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, readFile(path)).WillOnce(Return("123"));
    EXPECT_CALL(*mockFileSystem, writeFile(path, "14")).Times(1);
    Tests::replaceFileSystem(std::move(mockFileSystem));
    Common::PersistentValue<int> value(pathToVarDir,valueName, defaultValue);
    value.setValue(14);
    auto errorValue = value.getError();
    EXPECT_EQ(errorValue, "");
    ASSERT_EQ(value.getValue(), 14);
}

TEST_F(TestPersistentValue, setUnsignedIntPersistentValue)
{
    std::string pathToVarDir = "var";
    std::string valueName = "aPersistedValue";
    std::string path = Common::FileSystem::join(pathToVarDir, "persist-" + valueName);
    unsigned int defaultValue = 10;
    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, exists(path)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, readFile(path)).WillOnce(Return("123"));
    EXPECT_CALL(*mockFileSystem, writeFile(path, "14")).Times(1);
    Tests::replaceFileSystem(std::move(mockFileSystem));
    Common::PersistentValue<unsigned int> value(pathToVarDir,valueName, defaultValue);
    value.setValue(14);
    auto errorValue = value.getError();
    EXPECT_EQ(errorValue, "");
    ASSERT_EQ(value.getValue(), 14);
}

TEST_F(TestPersistentValue, setFloatPersistentValue)
{
    std::string pathToVarDir = "var";
    std::string valueName = "aPersistedValue";
    std::string path = Common::FileSystem::join(pathToVarDir, "persist-" + valueName);
    float defaultValue = 10.2;
    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, exists(path)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, readFile(path)).WillOnce(Return("14.2"));
    EXPECT_CALL(*mockFileSystem, writeFile(path, "14.2")).Times(1);
    Tests::replaceFileSystem(std::move(mockFileSystem));
    Common::PersistentValue<float> value(pathToVarDir,valueName, defaultValue);
    value.setValue(14.2);
    float difference = abs(value.getValue() - 14.2);
    float epsilon = 0.0001;
    auto errorValue = value.getError();
    EXPECT_EQ(errorValue, "");
    ASSERT_TRUE(difference < epsilon);
}

TEST_F(TestPersistentValue, setStringPersistentValue)
{
    std::string pathToVarDir = "var";
    std::string valueName = "aPersistedValue";
    std::string path = Common::FileSystem::join(pathToVarDir, "persist-" + valueName);
    std::string defaultValue = "someValue";
    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, exists(path)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, readFile(path)).WillOnce(Return("some value already there"));
    EXPECT_CALL(*mockFileSystem, writeFile(path, "another value")).Times(1);
    Tests::replaceFileSystem(std::move(mockFileSystem));
    Common::PersistentValue<std::string> value(pathToVarDir,valueName, defaultValue);
    value.setValue("another value");
    auto errorValue = value.getError();
    EXPECT_EQ(errorValue, "");
    ASSERT_EQ(value.getValue(), "another value");
}

TEST_F(TestPersistentValue, filesystemExceptionsOutputToStdErr)
{
    std::string pathToVarDir = "var";
    std::string valueName = "aPersistedValue";
    std::string path = Common::FileSystem::join(pathToVarDir, "persist-" + valueName);
    std::string defaultValue = "someValue";
    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, exists(path)).WillOnce(Throw(std::runtime_error("TEST")));
    EXPECT_CALL(*mockFileSystem, writeFile(path, defaultValue)).WillOnce(Throw(std::runtime_error("TEST")));
    Tests::replaceFileSystem(std::move(mockFileSystem));
    testing::internal::CaptureStderr();
    {
        Common::PersistentValue<std::string> value(pathToVarDir, valueName, defaultValue);
        ASSERT_EQ(value.getValue(), defaultValue);
    }
    std::string logMessage = internal::GetCapturedStderr();
    ASSERT_THAT(logMessage, ::testing::HasSubstr("ERROR Failed to save value to var/persist-aPersistedValue with error TEST" ));
}

TEST_F(TestPersistentValue, errorValueCanBeSetAndRetrieved)
{
    const std::string expectedStr = "NoPermission";
    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, exists(_)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, readFile(_)).WillOnce(Throw(Common::FileSystem::IPermissionDeniedException(expectedStr)));
    EXPECT_CALL(*mockFileSystem, writeFile(_, _)).Times(1);
    Tests::replaceFileSystem(std::move(mockFileSystem));
    Common::PersistentValue<std::string> value("", "", "");
    EXPECT_STREQ(expectedStr.c_str(), value.getError().c_str());
}

TEST_F(TestPersistentValue, testSetValueAndForceStoreWritesOnSetAndDestruction)
{
    std::string pathToVarDir = "var";
    std::string valueName = "aPersistedValue";
    std::string path = Common::FileSystem::join(pathToVarDir, "persist-" + valueName);
    int defaultValue = 10;
    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, exists(path)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, readFile(path)).WillOnce(Return("123"));
    // expect write 1 from the setValueAndForceStore(1) call
    EXPECT_CALL(*mockFileSystem, writeFile(path, "1")).Times(1);
    // expect write 2 from destruction after calling setValue(2)
    EXPECT_CALL(*mockFileSystem, writeFile(path, "2")).Times(1);
    Tests::replaceFileSystem(std::move(mockFileSystem));
    Common::PersistentValue<int> value(pathToVarDir,valueName, defaultValue);
    auto errorValue = value.getError();
    EXPECT_EQ(errorValue, "");
    value.setValueAndForceStore(1);
    ASSERT_EQ(value.getValue(), 1);
    value.setValue(2);
    ASSERT_EQ(value.getValue(), 2);
}