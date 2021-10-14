/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/Helpers/MockFileSystem.h>
#include "MockSophosJournal.h"
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/Helpers/FileSystemReplaceAndRestore.h>
#include <gtest/gtest.h>
#include <modules/EventJournalWrapperImpl/EventJournalReaderWrapper.h>
#include <modules/EventJournalWrapperImpl/Logger.h>
#include <tests/googletest/googlemock/include/gmock/gmock-matchers.h>

using namespace ::testing;

class TestEventJournalReaderWrapper : public ::testing::Test
{
public:
    void validJsonRequest()
    {
        return;
    }
};

// Tests for function: getCurrentJRLForId
TEST_F(TestEventJournalReaderWrapper, GetCurrentJRLForIdReturnsEmptyWhenFilepathNotAFile) // NOLINT
{
    std::string  idFilePath = "not_a_filepath";
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{ mockFileSystem });

    std::shared_ptr<Sophos::Journal::ViewInterface> mockViewInterface;
    MockJournalViewInterface *mockJournalViewInterface = new StrictMock<MockJournalViewInterface>();

    std::shared_ptr<Sophos::Journal::HelperInterface> testHelperInterface;
    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = new Common::EventJournalWrapper::Reader(testHelperInterface);
    EXPECT_CALL(*mockJournalViewInterface, GetJournalView(sub, jrl)).WillOnce(Return(mockViewInterface));

    EXPECT_CALL(*mockFileSystem, isFile(idFilePath)).WillOnce(Return(false));
    ASSERT_EQ(testReader->getCurrentJRLForId(idFilePath), "");

    EXPECT_EQ(mockViewInterface.use_count(), 1);
    EXPECT_EQ(testHelperInterface.use_count(), 1);
    EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockViewInterface.get()));
    EXPECT_TRUE(Mock::VerifyAndClearExpectations(testHelperInterface.get()));
}

TEST_F(TestEventJournalReaderWrapper, GetCurrentJRLForIdReturnsContentsWhenFilepathIsAFile) // NOLINT
{
    std::string idFilePath = "is_a_filepath";
    std::string expectedResult = "JRL token";
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{ mockFileSystem });

    std::shared_ptr<Sophos::Journal::ViewInterface> mockViewInterface;
    MockJournalViewInterface *mockJournalViewInterface = new StrictMock<MockJournalViewInterface>();

    std::shared_ptr<Sophos::Journal::HelperInterface> testHelperInterface;
    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = new Common::EventJournalWrapper::Reader(testHelperInterface);
    EXPECT_CALL(*mockJournalViewInterface, GetJournalView(sub, jrl)).WillOnce(Return(mockViewInterface));

    EXPECT_CALL(*mockFileSystem, isFile(idFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, readFile(idFilePath)).WillOnce(Return(expectedResult));
    ASSERT_EQ(testReader->getCurrentJRLForId(idFilePath), expectedResult);
}

TEST_F(TestEventJournalReaderWrapper, GetCurrentJRLForIdLogsWarnWhenFileFailsToRead) // NOLINT
{
    std::string idFilePath = "not_a_filepath";
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{ mockFileSystem });
    Common::Logging::ConsoleLoggingSetup consoleLogger;
    testing::internal::CaptureStderr();

    std::shared_ptr<Sophos::Journal::ViewInterface> mockViewInterface;
    MockJournalViewInterface *mockJournalViewInterface = new StrictMock<MockJournalViewInterface>();

    std::shared_ptr<Sophos::Journal::HelperInterface> testHelperInterface;
    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = new Common::EventJournalWrapper::Reader(testHelperInterface);
    EXPECT_CALL(*mockJournalViewInterface, GetJournalView(sub, jrl)).WillOnce(Return(mockViewInterface));

    EXPECT_CALL(*mockFileSystem, isFile(idFilePath)).WillOnce(Throw(Common::FileSystem::IFileSystemException ("Test exception")));
    ASSERT_EQ(testReader->getCurrentJRLForId(idFilePath), "");

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Failed to read jrl file not_a_filepath with error: Test exception"));
}

// Tests for function: updateJrl
TEST_F(TestEventJournalReaderWrapper, UpdateJrlDoesNothingWhenFilepathIsEmpty) // NOLINT
{
    std::string idFilePath = "";
    std::string newJrl = "New token";

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{ mockFileSystem });

    std::shared_ptr<Sophos::Journal::ViewInterface> mockViewInterface;
    MockJournalViewInterface *mockJournalViewInterface = new StrictMock<MockJournalViewInterface>();

    std::shared_ptr<Sophos::Journal::HelperInterface> testHelperInterface;
    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = new Common::EventJournalWrapper::Reader(testHelperInterface);
    EXPECT_CALL(*mockJournalViewInterface, GetJournalView(sub, jrl)).WillOnce(Return(mockViewInterface));

    // Nothing done in function
    EXPECT_CALL(*mockFileSystem, isFile(idFilePath)).Times(0);
    ASSERT_NO_THROW(testReader->updateJrl(idFilePath, newJrl));
}

TEST_F(TestEventJournalReaderWrapper, UpdateJrlRemovesOldJrlFileAndWritesNewFileWhenFilepathExists) // NOLINT
{
    std::string idFilePath = "JRL-filepath";
    std::string newJrl = "New token";

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{ mockFileSystem });

    std::shared_ptr<Sophos::Journal::ViewInterface> mockViewInterface;
    MockJournalViewInterface *mockJournalViewInterface = new StrictMock<MockJournalViewInterface>();

    std::shared_ptr<Sophos::Journal::HelperInterface> testHelperInterface;
    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = new Common::EventJournalWrapper::Reader(testHelperInterface);

    EXPECT_CALL(*mockFileSystem, isFile(idFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFile(idFilePath));
    EXPECT_CALL(*mockFileSystem, writeFile(idFilePath, newJrl));
    EXPECT_CALL(*mockJournalViewInterface, GetJournalView(sub, jrl)).WillOnce(Return(mockViewInterface));

    ASSERT_NO_THROW(testReader->updateJrl(idFilePath, newJrl));
}

TEST_F(TestEventJournalReaderWrapper, UpdateJrlWritesNewFileWhenFilepathDoesNotExist) // NOLINT
{
    std::string idFilePath = "JRL-filepath";
    std::string newJrl = "New token";

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{ mockFileSystem });

    std::shared_ptr<Sophos::Journal::ViewInterface> mockViewInterface;
    MockJournalViewInterface *mockJournalViewInterface = new StrictMock<MockJournalViewInterface>();

    std::shared_ptr<Sophos::Journal::HelperInterface> testHelperInterface;
    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = new Common::EventJournalWrapper::Reader(testHelperInterface);

    EXPECT_CALL(*mockFileSystem, isFile(idFilePath)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, removeFile(idFilePath)).Times(0);
    EXPECT_CALL(*mockFileSystem, writeFile(idFilePath, newJrl));
    EXPECT_CALL(*mockJournalViewInterface, GetJournalView(sub, jrl)).WillOnce(Return(mockViewInterface));

    ASSERT_NO_THROW(testReader->updateJrl(idFilePath, newJrl));
}

TEST_F(TestEventJournalReaderWrapper, UpdateJrlLogsWarnWhenFileFailsToRead) // NOLINT
{
    std::string idFilePath = "not_a_filepath";
    std::string newJrl = "New token";

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{ mockFileSystem });
    Common::Logging::ConsoleLoggingSetup consoleLogger;
    testing::internal::CaptureStderr();

    std::shared_ptr<Sophos::Journal::ViewInterface> mockViewInterface;
    MockJournalViewInterface *mockJournalViewInterface = new StrictMock<MockJournalViewInterface>();

    std::shared_ptr<Sophos::Journal::HelperInterface> testHelperInterface;
    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = new Common::EventJournalWrapper::Reader(testHelperInterface);
    EXPECT_CALL(*mockJournalViewInterface, GetJournalView(sub, jrl)).WillOnce(Return(mockViewInterface));

    EXPECT_CALL(*mockFileSystem, isFile(idFilePath)).WillOnce(Throw(Common::FileSystem::IFileSystemException ("Test exception")));
    ASSERT_NO_THROW(testReader->updateJrl(idFilePath, newJrl));

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Failed to update jrl file not_a_filepath with error: Test exception"));
}