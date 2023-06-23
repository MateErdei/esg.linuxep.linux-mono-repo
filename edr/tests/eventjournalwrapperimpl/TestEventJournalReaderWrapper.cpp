// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/Helpers/MockFileSystem.h>
#include "MockSophosJournal.h"
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/Helpers/FileSystemReplaceAndRestore.h>
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <modules/EventJournalWrapperImpl/EventJournalReaderWrapper.h>
#include <modules/EventJournalWrapperImpl/Logger.h>

using namespace ::testing;

class TestEventJournalReaderWrapper : public ::testing::Test
{};

// Tests for function: getCurrentJRLForId
TEST_F(TestEventJournalReaderWrapper, GetCurrentJRLForIdReturnsEmptyWhenFilepathNotAFile) // NOLINT
{
    std::string  idFilePath = "not_a_filepath";
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{ mockFileSystem });

    std::shared_ptr<Sophos::Journal::ViewInterface> mockViewInterface;
    std::shared_ptr<Sophos::Journal::HelperInterface> testHelperInterface;
    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = new Common::EventJournalWrapper::Reader(testHelperInterface);

    EXPECT_CALL(*mockFileSystem, isFile(idFilePath)).WillOnce(Return(false));
    ASSERT_EQ(testReader->getCurrentJRLForId(idFilePath), "");
}

TEST_F(TestEventJournalReaderWrapper, GetCurrentJRLForIdReturnsContentsWhenFilepathIsAFile) // NOLINT
{
    std::string idFilePath = "is_a_filepath";
    std::string expectedResult = "JRL token";
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{ mockFileSystem });

    std::shared_ptr<Sophos::Journal::ViewInterface> mockViewInterface;
    std::shared_ptr<Sophos::Journal::HelperInterface> testHelperInterface;
    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = new Common::EventJournalWrapper::Reader(testHelperInterface);

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
    std::shared_ptr<Sophos::Journal::HelperInterface> testHelperInterface;
    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = new Common::EventJournalWrapper::Reader(testHelperInterface);

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
    std::shared_ptr<Sophos::Journal::HelperInterface> testHelperInterface;
    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = new Common::EventJournalWrapper::Reader(testHelperInterface);

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
    std::shared_ptr<Sophos::Journal::HelperInterface> testHelperInterface;
    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = new Common::EventJournalWrapper::Reader(testHelperInterface);

    EXPECT_CALL(*mockFileSystem, isFile(idFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFile(idFilePath));
    EXPECT_CALL(*mockFileSystem, writeFile(idFilePath, newJrl));

    ASSERT_NO_THROW(testReader->updateJrl(idFilePath, newJrl));
}

TEST_F(TestEventJournalReaderWrapper, UpdateJrlWritesNewFileWhenFilepathDoesNotExist) // NOLINT
{
    std::string idFilePath = "JRL-filepath";
    std::string newJrl = "New token";

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{ mockFileSystem });

    std::shared_ptr<Sophos::Journal::ViewInterface> mockViewInterface;
    std::shared_ptr<Sophos::Journal::HelperInterface> testHelperInterface;
    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = new Common::EventJournalWrapper::Reader(testHelperInterface);

    EXPECT_CALL(*mockFileSystem, isFile(idFilePath)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, removeFile(idFilePath)).Times(0);
    EXPECT_CALL(*mockFileSystem, writeFile(idFilePath, newJrl));

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
    std::shared_ptr<Sophos::Journal::HelperInterface> testHelperInterface;
    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = new Common::EventJournalWrapper::Reader(testHelperInterface);

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
    std::shared_ptr<Sophos::Journal::HelperInterface> testHelperInterface;
    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = new Common::EventJournalWrapper::Reader(testHelperInterface);

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
    std::shared_ptr<Sophos::Journal::HelperInterface> testHelperInterface;
    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = new Common::EventJournalWrapper::Reader(testHelperInterface);

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
    std::shared_ptr<Sophos::Journal::HelperInterface> testHelperInterface;
    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = new Common::EventJournalWrapper::Reader(testHelperInterface);

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
    std::shared_ptr<Sophos::Journal::HelperInterface> testHelperInterface;
    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = new Common::EventJournalWrapper::Reader(testHelperInterface);

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
    std::shared_ptr<Sophos::Journal::HelperInterface> testHelperInterface;
    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = new Common::EventJournalWrapper::Reader(testHelperInterface);

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
    std::shared_ptr<Sophos::Journal::HelperInterface> testHelperInterface;
    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = new Common::EventJournalWrapper::Reader(testHelperInterface);

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
    std::shared_ptr<Sophos::Journal::HelperInterface> testHelperInterface;
    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = new Common::EventJournalWrapper::Reader(testHelperInterface);

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
    std::shared_ptr<Sophos::Journal::HelperInterface> testHelperInterface;
    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = new Common::EventJournalWrapper::Reader(testHelperInterface);

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
    std::shared_ptr<Sophos::Journal::HelperInterface> testHelperInterface;
    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = new Common::EventJournalWrapper::Reader(testHelperInterface);

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
    std::shared_ptr<Sophos::Journal::HelperInterface> testHelperInterface;
    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = new Common::EventJournalWrapper::Reader(testHelperInterface);

    EXPECT_CALL(*mockFileSystem, isFile(idFilePath)).WillOnce(Throw(Common::FileSystem::IFileSystemException("Test exception")));
    ASSERT_NO_THROW(testReader->clearJRLFile(idFilePath));

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Failed to remove file not_a_filepath with error: Test exception"));
}

TEST_F(TestEventJournalReaderWrapper, getEntriesCanReadMultipleLongRecords)
{
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{ mockFileSystem });
    Common::Logging::ConsoleLoggingSetup consoleLogger;

    const int size = 9000;
    std::shared_ptr<MockJournalHelperInterface> mockJournalHelperPtr  = std::make_shared<MockJournalHelperInterface>();
    std::shared_ptr<MockJournalViewInterface> mockJournalViewPtr = std::make_shared<MockJournalViewInterface>();
    EXPECT_CALL(*mockJournalHelperPtr, GetJournalView(_,_,_)).WillOnce(Return(mockJournalViewPtr));

    std::shared_ptr<MockJournalEntryInterface> mockJournalEntryInterface = std::make_shared<MockJournalEntryInterface>();
    std::vector<std::byte> bytes(size, std::byte{0x33});
    EXPECT_CALL(*mockJournalEntryInterface, GetDataSize()).WillRepeatedly(Return(size));
    EXPECT_CALL(*mockJournalEntryInterface, GetData()).WillRepeatedly(Return(bytes.data()));

    std::shared_ptr<MockImplementationInterface> beginImplInterface = std::make_shared<MockImplementationInterface>();
    testing::Mock::AllowLeak(beginImplInterface.get());
    std::shared_ptr<MockImplementationInterface> endInterface = std::make_shared<MockImplementationInterface>();
    testing::Mock::AllowLeak(endInterface.get());

    EXPECT_CALL(*beginImplInterface, Equals())
        .WillOnce(Return(false))
        .WillOnce(Return(false))
        .WillOnce(Return(false))
        .WillOnce(Return(false))
        .WillOnce(Return(false))
        .WillRepeatedly(Return(true));

    EXPECT_CALL(*endInterface, Equals())
        .WillOnce(Return(false))
        .WillOnce(Return(false))
        .WillOnce(Return(false))
        .WillOnce(Return(false))
        .WillOnce(Return(false))
        .WillRepeatedly(Return(true));

    EXPECT_CALL(*beginImplInterface, Copy()).WillRepeatedly(Return(beginImplInterface));
    EXPECT_CALL(*endInterface, Copy()).WillRepeatedly(Return(endInterface));

    std::shared_ptr<Sophos::Journal::ViewInterface::EntryInterface> sharedPtrToEntryInterfaceMock = mockJournalEntryInterface;
    EXPECT_CALL(*beginImplInterface, Dereference()).WillRepeatedly(ReturnRef(sharedPtrToEntryInterfaceMock));
    MockJournalViewInterface::ConstIterator journalIterator(beginImplInterface);

    EXPECT_CALL(*beginImplInterface, PrefixIncrement())
        .WillOnce(ReturnRef(*beginImplInterface))
        .WillOnce(ReturnRef(*beginImplInterface))
        .WillOnce(ReturnRef(*beginImplInterface))
        .WillOnce(ReturnRef(*beginImplInterface))
        .WillOnce(ReturnRef(*endInterface));

    EXPECT_CALL(*mockJournalViewPtr, cbegin()).WillRepeatedly(Return(journalIterator));
    EXPECT_CALL(*mockJournalViewPtr, cend()).WillRepeatedly(Return(journalIterator));

    FILETIME ftime{};
    EXPECT_CALL(*mockJournalEntryInterface, GetProducerUniqueId()).WillRepeatedly(Return(1111));
    EXPECT_CALL(*mockJournalEntryInterface, GetTimestamp()).WillRepeatedly(Return(ftime));
    EXPECT_CALL(*beginImplInterface, GetJournalResourceLocator()).WillRepeatedly(Return("jrl"));

    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = std::make_unique<Common::EventJournalWrapper::Reader>(mockJournalHelperPtr);

    std::vector<Common::EventJournalWrapper::Subject> subjectFilter = {Common::EventJournalWrapper::Subject::Detections};
    uint64_t startTime = 0;
    uint64_t endTime = 10000;
    uint32_t maxMemoryThreshold = 10000000;
    bool moreAvailable = false;
    auto results = testReader->getEntries(subjectFilter, startTime, endTime, maxMemoryThreshold, moreAvailable);
    ASSERT_EQ(results.size(), 5);
    ASSERT_FALSE(moreAvailable);
}

TEST_F(TestEventJournalReaderWrapper, getEntriesStopsReadingAfterHittingMemoryLimit)
{
    auto mockFileSystem = std::make_unique<testing::StrictMock<MockFileSystem>>();
    Tests::replaceFileSystem(std::move(mockFileSystem));

    Common::Logging::ConsoleLoggingSetup consoleLogger;

    const int size = 9000;
    std::shared_ptr<MockJournalHelperInterface> mockJournalHelperPtr  = std::make_shared<MockJournalHelperInterface>();
    std::shared_ptr<MockJournalViewInterface> mockJournalViewPtr = std::make_shared<MockJournalViewInterface>();
    EXPECT_CALL(*mockJournalHelperPtr, GetJournalView(_,_,_)).WillOnce(Return(mockJournalViewPtr));

    std::shared_ptr<MockJournalEntryInterface> mockJournalEntryInterface = std::make_shared<MockJournalEntryInterface>();
    std::vector<std::byte> bytes(size, std::byte{0x33});
    EXPECT_CALL(*mockJournalEntryInterface, GetDataSize()).WillRepeatedly(Return(size));
    EXPECT_CALL(*mockJournalEntryInterface, GetData()).WillRepeatedly(Return(bytes.data()));

    std::shared_ptr<MockImplementationInterface> beginImplInterface = std::make_shared<MockImplementationInterface>();
    testing::Mock::AllowLeak(beginImplInterface.get());
    std::shared_ptr<MockImplementationInterface> endInterface = std::make_shared<MockImplementationInterface>();
    testing::Mock::AllowLeak(endInterface.get());

    EXPECT_CALL(*beginImplInterface, Equals())
        .WillOnce(Return(false))
        .WillOnce(Return(false))
        .WillOnce(Return(false))
        .WillOnce(Return(false))
        .WillOnce(Return(false))
        .WillRepeatedly(Return(true));

    EXPECT_CALL(*endInterface, Equals())
        .WillOnce(Return(false))
        .WillOnce(Return(false))
        .WillOnce(Return(false))
        .WillOnce(Return(false))
        .WillOnce(Return(false))
        .WillRepeatedly(Return(true));

    EXPECT_CALL(*beginImplInterface, Copy()).WillRepeatedly(Return(beginImplInterface));
    EXPECT_CALL(*endInterface, Copy()).WillRepeatedly(Return(endInterface));

    std::shared_ptr<Sophos::Journal::ViewInterface::EntryInterface> sharedPtrToEntryInterfaceMock = mockJournalEntryInterface;
    EXPECT_CALL(*beginImplInterface, Dereference()).WillRepeatedly(ReturnRef(sharedPtrToEntryInterfaceMock));
    MockJournalViewInterface::ConstIterator journalIterator(beginImplInterface);

    EXPECT_CALL(*beginImplInterface, PrefixIncrement())
        .WillOnce(ReturnRef(*beginImplInterface))
        .WillOnce(ReturnRef(*beginImplInterface))
        .WillOnce(ReturnRef(*beginImplInterface))
        .WillOnce(ReturnRef(*beginImplInterface))
        .WillOnce(ReturnRef(*endInterface));

    EXPECT_CALL(*mockJournalViewPtr, cbegin()).WillRepeatedly(Return(journalIterator));
    EXPECT_CALL(*mockJournalViewPtr, cend()).WillRepeatedly(Return(journalIterator));

    FILETIME ftime{};
    EXPECT_CALL(*mockJournalEntryInterface, GetProducerUniqueId()).WillRepeatedly(Return(1111));
    EXPECT_CALL(*mockJournalEntryInterface, GetTimestamp()).WillRepeatedly(Return(ftime));
    EXPECT_CALL(*beginImplInterface, GetJournalResourceLocator()).WillRepeatedly(Return("jrl"));

    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = std::make_unique<Common::EventJournalWrapper::Reader>(mockJournalHelperPtr);

    std::vector<Common::EventJournalWrapper::Subject> subjectFilter = {Common::EventJournalWrapper::Subject::Detections};
    uint64_t startTime = 0;
    uint64_t endTime = 10000;
    uint32_t maxMemoryThreshold = size * 2;
    bool moreAvailable = false;
    auto results = testReader->getEntries(subjectFilter, startTime, endTime, maxMemoryThreshold, moreAvailable);
    ASSERT_EQ(results.size(), 2);
    ASSERT_TRUE(moreAvailable);

    Mock::VerifyAndClearExpectations(beginImplInterface.get());
    Mock::VerifyAndClearExpectations(endInterface.get());
}