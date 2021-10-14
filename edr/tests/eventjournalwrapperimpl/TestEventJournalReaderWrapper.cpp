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

// Tests for function: getCurrentJRLAttemptsForId
TEST_F(TestEventJournalReaderWrapper, GetCurrentJRLAttemptsForIdReadsFileAndReturnsItsValue) // NOLINT
{
    std::string idFilePath = "filepath";
    std::string testAttempts = "10";
    u_int32_t testAttemptsAsInt = 10;

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });

    std::shared_ptr<Sophos::Journal::ViewInterface> mockViewInterface;
    MockJournalViewInterface* mockJournalViewInterface = new StrictMock<MockJournalViewInterface>();

    std::shared_ptr<Sophos::Journal::HelperInterface> testHelperInterface;
    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = new Common::EventJournalWrapper::Reader(testHelperInterface);
    EXPECT_CALL(*mockJournalViewInterface, GetJournalView(sub, jrl)).WillOnce(Return(mockViewInterface));

    EXPECT_CALL(*mockFileSystem, isFile(idFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, readFile(idFilePath)).WillOnce(Return(testAttempts));
    ASSERT_EQ(testReader->getCurrentJRLAttemptsForId(idFilePath), testAttemptsAsInt);
}

TEST_F(TestEventJournalReaderWrapper, GetCurrentJRLAttemptsForIdReturnsZeroWithFilepathIsNotAFile) // NOLINT
{
    std::string idFilePath = "filepath";
    u_int32_t expectedResult = 0;

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });

    std::shared_ptr<Sophos::Journal::ViewInterface> mockViewInterface;
    MockJournalViewInterface* mockJournalViewInterface = new StrictMock<MockJournalViewInterface>();

    std::shared_ptr<Sophos::Journal::HelperInterface> testHelperInterface;
    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = new Common::EventJournalWrapper::Reader(testHelperInterface);
    EXPECT_CALL(*mockJournalViewInterface, GetJournalView(sub, jrl)).WillOnce(Return(mockViewInterface));

    EXPECT_CALL(*mockFileSystem, isFile(idFilePath)).WillOnce(Return(false));
    ASSERT_EQ(testReader->getCurrentJRLAttemptsForId(idFilePath), expectedResult);
}

TEST_F(TestEventJournalReaderWrapper, GetCurrentJRLAttemptsLogsWarnOnFileSystemExceptionAndReturnsZero) // NOLINT
{
    std::string idFilePath = "not_a_filepath";
    u_int32_t expectedResult = 0;

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
    ASSERT_EQ(testReader->getCurrentJRLAttemptsForId(idFilePath), expectedResult);

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Failed to read jrl tracker file not_a_filepath with error: Test exception"));
}

TEST_F(TestEventJournalReaderWrapper, GetCurrentJRLAttemptsLogsWarnOnGenericExceptionAndReturnsZero) // NOLINT
{
    std::string idFilePath = "not_a_filepath";
    u_int32_t expectedResult = 0;
    std::exception testException;

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

    EXPECT_CALL(*mockFileSystem, isFile(idFilePath)).WillOnce(Throw(testException));
    ASSERT_EQ(testReader->getCurrentJRLAttemptsForId(idFilePath), expectedResult);

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Failed to convert contents of tracker file not_a_filepath to int with error: std::exception"));
}

// Tests for function: updateJRLAttempts
TEST_F(TestEventJournalReaderWrapper, UpdateJRLAttemptsRemovesExistingFileAndWritesANewOneWhenFilepathExists) // NOLINT
{
    std::string idFilePath = "filepath";
    u_int32_t testAttempts = 10;
    std::string  testAttemptsAsString = "10";

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });

    std::shared_ptr<Sophos::Journal::ViewInterface> mockViewInterface;
    MockJournalViewInterface* mockJournalViewInterface = new StrictMock<MockJournalViewInterface>();

    std::shared_ptr<Sophos::Journal::HelperInterface> testHelperInterface;
    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = new Common::EventJournalWrapper::Reader(testHelperInterface);
    EXPECT_CALL(*mockJournalViewInterface, GetJournalView(sub, jrl)).WillOnce(Return(mockViewInterface));

    EXPECT_CALL(*mockFileSystem, isFile(idFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFile(idFilePath));
    EXPECT_CALL(*mockFileSystem, writeFile(idFilePath, testAttemptsAsString));
    ASSERT_NO_THROW(testReader->updateJRLAttempts(idFilePath, testAttempts));
}

TEST_F(TestEventJournalReaderWrapper, UpdateJRLAttemptsWritesANewFileWhenFilepathDoesNotExist) // NOLINT
{
    std::string idFilePath = "filepath";
    u_int32_t testAttempts = 10;
    std::string  testAttemptsAsString = "10";

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });

    std::shared_ptr<Sophos::Journal::ViewInterface> mockViewInterface;
    MockJournalViewInterface* mockJournalViewInterface = new StrictMock<MockJournalViewInterface>();

    std::shared_ptr<Sophos::Journal::HelperInterface> testHelperInterface;
    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = new Common::EventJournalWrapper::Reader(testHelperInterface);
    EXPECT_CALL(*mockJournalViewInterface, GetJournalView(sub, jrl)).WillOnce(Return(mockViewInterface));

    EXPECT_CALL(*mockFileSystem, isFile(idFilePath)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, removeFile(idFilePath)).Times(0);
    EXPECT_CALL(*mockFileSystem, writeFile(idFilePath, testAttemptsAsString));
    ASSERT_NO_THROW(testReader->updateJRLAttempts(idFilePath, testAttempts));
}

TEST_F(TestEventJournalReaderWrapper, UpdateJRLAttemptsLogsWarnOnFileSystemException) // NOLINT
{
    std::string idFilePath = "not_a_filepath";
    u_int32_t testAttempts = 10;

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

    EXPECT_CALL(*mockFileSystem, isFile(idFilePath)).WillOnce(Throw(Common::FileSystem::IFileSystemException("Test exception")));
    ASSERT_NO_THROW(testReader->updateJRLAttempts(idFilePath, testAttempts));

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Failed to update jrl tracker file not_a_filepath with error: Test exception"));
}

// Tests for function: clearJRLFile
TEST_F(TestEventJournalReaderWrapper, ClearJRLFileRemovesExistingFileIfFilepathExists) // NOLINT
{
    std::string idFilePath = "filepath";

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });

    std::shared_ptr<Sophos::Journal::ViewInterface> mockViewInterface;
    MockJournalViewInterface* mockJournalViewInterface = new StrictMock<MockJournalViewInterface>();

    std::shared_ptr<Sophos::Journal::HelperInterface> testHelperInterface;
    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = new Common::EventJournalWrapper::Reader(testHelperInterface);
    EXPECT_CALL(*mockJournalViewInterface, GetJournalView(sub, jrl)).WillOnce(Return(mockViewInterface));

    EXPECT_CALL(*mockFileSystem, isFile(idFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFile(idFilePath));
    ASSERT_NO_THROW(testReader->clearJRLFile(idFilePath));
}

TEST_F(TestEventJournalReaderWrapper, ClearJRLFileDoesNothingIfFilepathDoesNotExist) // NOLINT
{
    std::string idFilePath = "filepath";

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });

    std::shared_ptr<Sophos::Journal::ViewInterface> mockViewInterface;
    MockJournalViewInterface* mockJournalViewInterface = new StrictMock<MockJournalViewInterface>();

    std::shared_ptr<Sophos::Journal::HelperInterface> testHelperInterface;
    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = new Common::EventJournalWrapper::Reader(testHelperInterface);
    EXPECT_CALL(*mockJournalViewInterface, GetJournalView(sub, jrl)).WillOnce(Return(mockViewInterface));

    EXPECT_CALL(*mockFileSystem, isFile(idFilePath)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, removeFile(idFilePath)).Times(0);
    ASSERT_NO_THROW(testReader->clearJRLFile(idFilePath));
}

TEST_F(TestEventJournalReaderWrapper, ClearJRLFileLogsWarnOnFileSystemException) // NOLINT
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

    EXPECT_CALL(*mockFileSystem, isFile(idFilePath)).WillOnce(Throw(Common::FileSystem::IFileSystemException("Test exception")));
    ASSERT_NO_THROW(testReader->clearJRLFile(idFilePath));

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Failed to remove file not_a_filepath with error: Test exception"));
}