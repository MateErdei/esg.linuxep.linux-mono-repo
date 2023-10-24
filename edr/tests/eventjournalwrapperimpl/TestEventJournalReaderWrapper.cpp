// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include "MockSophosJournal.h"

#include "Common/FileSystem/IFileSystemException.h"
#include "Common/Logging/ConsoleLoggingSetup.h"

#ifdef SPL_BAZEL
#include "base/tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "base/tests/Common/Helpers/MockFileSystem.h"
#else
#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/MockFileSystem.h"
#endif

#include "EventJournalWrapperImpl/EventJournalReaderWrapper.h"

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

using namespace ::testing;

class TestEventJournalReaderWrapper : public ::testing::Test
{};

// Tests for function: getCurrentJRLForId
TEST_F(TestEventJournalReaderWrapper, GetCurrentJRLForIdReturnsEmptyWhenFilepathNotAFile)
{
    std::string idFilePath = "not_a_filepath";
    auto mockFileSystem = std::make_unique<::testing::StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, isFile(idFilePath)).WillOnce(Return(false));
    Tests::replaceFileSystem(std::move(mockFileSystem));

    std::shared_ptr<Sophos::Journal::ViewInterface> mockViewInterface;
    std::shared_ptr<Sophos::Journal::HelperInterface> testHelperInterface;
    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = std::make_unique<Common::EventJournalWrapper::Reader>(testHelperInterface);

    ASSERT_EQ(testReader->getCurrentJRLForId(idFilePath), "");
}

TEST_F(TestEventJournalReaderWrapper, GetCurrentJRLForIdReturnsContentsWhenFilepathIsAFile)
{
    std::string idFilePath = "is_a_filepath";
    std::string expectedResult = "JRL token";
    auto mockFileSystem = std::make_unique<::testing::StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, isFile(idFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, readFile(idFilePath)).WillOnce(Return(expectedResult));
    Tests::replaceFileSystem(std::move(mockFileSystem));

    std::shared_ptr<Sophos::Journal::ViewInterface> mockViewInterface;
    std::shared_ptr<Sophos::Journal::HelperInterface> testHelperInterface;
    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = std::make_unique<Common::EventJournalWrapper::Reader>(testHelperInterface);

    ASSERT_EQ(testReader->getCurrentJRLForId(idFilePath), expectedResult);
}

TEST_F(TestEventJournalReaderWrapper, GetCurrentJRLForIdLogsWarnWhenFileFailsToRead)
{
    std::string idFilePath = "not_a_filepath";
    auto mockFileSystem = std::make_unique<::testing::StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, isFile(idFilePath)).WillOnce(Throw(Common::FileSystem::IFileSystemException ("Test exception")));
    Tests::replaceFileSystem(std::move(mockFileSystem));
    Common::Logging::ConsoleLoggingSetup consoleLogger;
    testing::internal::CaptureStderr();

    std::shared_ptr<Sophos::Journal::ViewInterface> mockViewInterface;
    std::shared_ptr<Sophos::Journal::HelperInterface> testHelperInterface;
    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = std::make_unique<Common::EventJournalWrapper::Reader>(testHelperInterface);

    ASSERT_EQ(testReader->getCurrentJRLForId(idFilePath), "");

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Failed to read jrl file not_a_filepath with error: Test exception"));
}

// Tests for function: updateJrl
TEST_F(TestEventJournalReaderWrapper, UpdateJrlDoesNothingWhenFilepathIsEmpty)
{
    std::string idFilePath{};
    std::string newJrl = "New token";

    auto mockFileSystem = std::make_unique<::testing::StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, isFile(idFilePath)).Times(0);
    Tests::replaceFileSystem(std::move(mockFileSystem));

    std::shared_ptr<Sophos::Journal::ViewInterface> mockViewInterface;
    std::shared_ptr<Sophos::Journal::HelperInterface> testHelperInterface;
    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = std::make_unique<Common::EventJournalWrapper::Reader>(testHelperInterface);

    // Nothing done in function
    ASSERT_NO_THROW(testReader->updateJrl(idFilePath, newJrl));
}

TEST_F(TestEventJournalReaderWrapper, UpdateJrlRemovesOldJrlFileAndWritesNewFileWhenFilepathExists)
{
    std::string idFilePath = "JRL-filepath";
    std::string newJrl = "New token";

    auto mockFileSystem = std::make_unique<::testing::StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, isFile(idFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFile(idFilePath));
    EXPECT_CALL(*mockFileSystem, writeFile(idFilePath, newJrl));
    Tests::replaceFileSystem(std::move(mockFileSystem));

    std::shared_ptr<Sophos::Journal::ViewInterface> mockViewInterface;
    std::shared_ptr<Sophos::Journal::HelperInterface> testHelperInterface;
    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = std::make_unique<Common::EventJournalWrapper::Reader>(testHelperInterface);


    ASSERT_NO_THROW(testReader->updateJrl(idFilePath, newJrl));
}

TEST_F(TestEventJournalReaderWrapper, UpdateJrlWritesNewFileWhenFilepathDoesNotExist)
{
    std::string idFilePath = "JRL-filepath";
    std::string newJrl = "New token";

    auto mockFileSystem = std::make_unique<::testing::StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, isFile(idFilePath)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, removeFile(idFilePath)).Times(0);
    EXPECT_CALL(*mockFileSystem, writeFile(idFilePath, newJrl));
    Tests::replaceFileSystem(std::move(mockFileSystem));

    std::shared_ptr<Sophos::Journal::HelperInterface> testHelperInterface;
    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = std::make_unique<Common::EventJournalWrapper::Reader>(testHelperInterface);

    ASSERT_NO_THROW(testReader->updateJrl(idFilePath, newJrl));
}

TEST_F(TestEventJournalReaderWrapper, UpdateJrlLogsWarnWhenFileFailsToRead)
{
    std::string idFilePath = "not_a_filepath";
    std::string newJrl = "New token";

    auto mockFileSystem = std::make_unique<::testing::StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, isFile(idFilePath)).WillOnce(Throw(Common::FileSystem::IFileSystemException ("Test exception")));
    Tests::replaceFileSystem(std::move(mockFileSystem));
    Common::Logging::ConsoleLoggingSetup consoleLogger;
    testing::internal::CaptureStderr();

    std::shared_ptr<Sophos::Journal::ViewInterface> mockViewInterface;
    std::shared_ptr<Sophos::Journal::HelperInterface> testHelperInterface;
    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = std::make_unique<Common::EventJournalWrapper::Reader>(testHelperInterface);

    ASSERT_NO_THROW(testReader->updateJrl(idFilePath, newJrl));

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Failed to update jrl file not_a_filepath with error: Test exception"));
}

// Tests for function: getCurrentJRLAttemptsForId
TEST_F(TestEventJournalReaderWrapper, GetCurrentJRLAttemptsForIdReadsFileAndReturnsItsValue)
{
    std::string idFilePath = "filepath";
    std::string testAttempts = "10";
    u_int32_t testAttemptsAsInt = 10;

    auto mockFileSystem = std::make_unique<::testing::StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, isFile(idFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, readFile(idFilePath)).WillOnce(Return(testAttempts));
    Tests::replaceFileSystem(std::move(mockFileSystem));

    std::shared_ptr<Sophos::Journal::ViewInterface> mockViewInterface;
    std::shared_ptr<Sophos::Journal::HelperInterface> testHelperInterface;
    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = std::make_unique<Common::EventJournalWrapper::Reader>(testHelperInterface);

    ASSERT_EQ(testReader->getCurrentJRLAttemptsForId(idFilePath), testAttemptsAsInt);
}

TEST_F(TestEventJournalReaderWrapper, GetCurrentJRLAttemptsForIdReturnsZeroWithFilepathIsNotAFile)
{
    std::string idFilePath = "filepath";
    u_int32_t expectedResult = 0;

    auto mockFileSystem = std::make_unique<::testing::StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, isFile(idFilePath)).WillOnce(Return(false));
    Tests::replaceFileSystem(std::move(mockFileSystem));

    std::shared_ptr<Sophos::Journal::ViewInterface> mockViewInterface;
    std::shared_ptr<Sophos::Journal::HelperInterface> testHelperInterface;
    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = std::make_unique<Common::EventJournalWrapper::Reader>(testHelperInterface);

    ASSERT_EQ(testReader->getCurrentJRLAttemptsForId(idFilePath), expectedResult);
}

TEST_F(TestEventJournalReaderWrapper, GetCurrentJRLAttemptsLogsWarnOnFileSystemExceptionAndReturnsZero)
{
    std::string idFilePath = "not_a_filepath";
    u_int32_t expectedResult = 0;

    auto mockFileSystem = std::make_unique<::testing::StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, isFile(idFilePath)).WillOnce(Throw(Common::FileSystem::IFileSystemException ("Test exception")));
    Tests::replaceFileSystem(std::move(mockFileSystem));
    Common::Logging::ConsoleLoggingSetup consoleLogger;
    testing::internal::CaptureStderr();

    std::shared_ptr<Sophos::Journal::ViewInterface> mockViewInterface;
    std::shared_ptr<Sophos::Journal::HelperInterface> testHelperInterface;
    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = std::make_unique<Common::EventJournalWrapper::Reader>(testHelperInterface);

    ASSERT_EQ(testReader->getCurrentJRLAttemptsForId(idFilePath), expectedResult);

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Failed to read jrl tracker file not_a_filepath with error: Test exception"));
}

TEST_F(TestEventJournalReaderWrapper, GetCurrentJRLAttemptsLogsWarnOnGenericExceptionAndReturnsZero)
{
    std::string idFilePath = "not_a_filepath";
    u_int32_t expectedResult = 0;
    std::exception testException;

    auto mockFileSystem = std::make_unique<::testing::StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, isFile(idFilePath)).WillOnce(Throw(testException));
    Tests::replaceFileSystem(std::move(mockFileSystem));
    Common::Logging::ConsoleLoggingSetup consoleLogger;
    testing::internal::CaptureStderr();

    std::shared_ptr<Sophos::Journal::ViewInterface> mockViewInterface;
    std::shared_ptr<Sophos::Journal::HelperInterface> testHelperInterface;
    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = std::make_unique<Common::EventJournalWrapper::Reader>(testHelperInterface);

    ASSERT_EQ(testReader->getCurrentJRLAttemptsForId(idFilePath), expectedResult);

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Failed to convert contents of tracker file not_a_filepath to int with error: std::exception"));
}

// Tests for function: updateJRLAttempts
TEST_F(TestEventJournalReaderWrapper, UpdateJRLAttemptsRemovesExistingFileAndWritesANewOneWhenFilepathExists)
{
    std::string idFilePath = "filepath";
    u_int32_t testAttempts = 10;
    std::string  testAttemptsAsString = "10";

    auto mockFileSystem = std::make_unique<::testing::StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, isFile(idFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFile(idFilePath));
    EXPECT_CALL(*mockFileSystem, writeFile(idFilePath, testAttemptsAsString));
    Tests::replaceFileSystem(std::move(mockFileSystem));

    std::shared_ptr<Sophos::Journal::ViewInterface> mockViewInterface;
    std::shared_ptr<Sophos::Journal::HelperInterface> testHelperInterface;
    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = std::make_unique<Common::EventJournalWrapper::Reader>(testHelperInterface);

    ASSERT_NO_THROW(testReader->updateJRLAttempts(idFilePath, testAttempts));
}

TEST_F(TestEventJournalReaderWrapper, UpdateJRLAttemptsWritesANewFileWhenFilepathDoesNotExist)
{
    std::string idFilePath = "filepath";
    u_int32_t testAttempts = 10;
    std::string  testAttemptsAsString = "10";

    auto mockFileSystem = std::make_unique<::testing::StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, isFile(idFilePath)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, removeFile(idFilePath)).Times(0);
    EXPECT_CALL(*mockFileSystem, writeFile(idFilePath, testAttemptsAsString));
    Tests::replaceFileSystem(std::move(mockFileSystem));

    std::shared_ptr<Sophos::Journal::ViewInterface> mockViewInterface;
    std::shared_ptr<Sophos::Journal::HelperInterface> testHelperInterface;
    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = std::make_unique<Common::EventJournalWrapper::Reader>(testHelperInterface);

    ASSERT_NO_THROW(testReader->updateJRLAttempts(idFilePath, testAttempts));
}

TEST_F(TestEventJournalReaderWrapper, UpdateJRLAttemptsLogsWarnOnFileSystemException)
{
    std::string idFilePath = "not_a_filepath";
    u_int32_t testAttempts = 10;

    auto mockFileSystem = std::make_unique<::testing::StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, isFile(idFilePath)).WillOnce(Throw(Common::FileSystem::IFileSystemException("Test exception")));
    Tests::replaceFileSystem(std::move(mockFileSystem));
    Common::Logging::ConsoleLoggingSetup consoleLogger;
    testing::internal::CaptureStderr();

    std::shared_ptr<Sophos::Journal::ViewInterface> mockViewInterface;
    std::shared_ptr<Sophos::Journal::HelperInterface> testHelperInterface;
    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = std::make_unique<Common::EventJournalWrapper::Reader>(testHelperInterface);

    ASSERT_NO_THROW(testReader->updateJRLAttempts(idFilePath, testAttempts));

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Failed to update jrl tracker file not_a_filepath with error: Test exception"));
}

// Tests for function: clearJRLFile
TEST_F(TestEventJournalReaderWrapper, ClearJRLFileRemovesExistingFileIfFilepathExists)
{
    std::string idFilePath = "filepath";

    auto mockFileSystem = std::make_unique<::testing::StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, isFile(idFilePath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFile(idFilePath));
    Tests::replaceFileSystem(std::move(mockFileSystem));

    std::shared_ptr<Sophos::Journal::ViewInterface> mockViewInterface;
    std::shared_ptr<Sophos::Journal::HelperInterface> testHelperInterface;
    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = std::make_unique<Common::EventJournalWrapper::Reader>(testHelperInterface);

    ASSERT_NO_THROW(testReader->clearJRLFile(idFilePath));
}

TEST_F(TestEventJournalReaderWrapper, ClearJRLFileDoesNothingIfFilepathDoesNotExist)
{
    std::string idFilePath = "filepath";

    auto mockFileSystem = std::make_unique<::testing::StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, isFile(idFilePath)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, removeFile(idFilePath)).Times(0);
    Tests::replaceFileSystem(std::move(mockFileSystem));

    std::shared_ptr<Sophos::Journal::ViewInterface> mockViewInterface;
    std::shared_ptr<Sophos::Journal::HelperInterface> testHelperInterface;
    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = std::make_unique<Common::EventJournalWrapper::Reader>(testHelperInterface);

    ASSERT_NO_THROW(testReader->clearJRLFile(idFilePath));
}

TEST_F(TestEventJournalReaderWrapper, ClearJRLFileLogsWarnOnFileSystemException)
{
    std::string idFilePath = "not_a_filepath";

    auto mockFileSystem = std::make_unique<::testing::StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, isFile(idFilePath)).WillOnce(Throw(Common::FileSystem::IFileSystemException("Test exception")));
    Tests::replaceFileSystem(std::move(mockFileSystem));
    Common::Logging::ConsoleLoggingSetup consoleLogger;
    testing::internal::CaptureStderr();

    std::shared_ptr<Sophos::Journal::ViewInterface> mockViewInterface;
    std::shared_ptr<Sophos::Journal::HelperInterface> testHelperInterface;
    Sophos::Journal::Subjects sub;
    Sophos::Journal::JRL jrl;
    auto testReader = std::make_unique<Common::EventJournalWrapper::Reader>(testHelperInterface);

    ASSERT_NO_THROW(testReader->clearJRLFile(idFilePath));

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Failed to remove file not_a_filepath with error: Test exception"));
}

TEST_F(TestEventJournalReaderWrapper, getEntriesCanReadMultipleLongRecords)
{
    auto mockFileSystem = std::make_unique<::testing::StrictMock<MockFileSystem>>();
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
    EXPECT_EQ(results.size(), 5);
    EXPECT_FALSE(moreAvailable);

    Mock::VerifyAndClearExpectations(beginImplInterface.get());
    Mock::VerifyAndClearExpectations(endInterface.get());
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
        .WillRepeatedly(Return(false));

    EXPECT_CALL(*beginImplInterface, Copy()).WillRepeatedly(Return(beginImplInterface));
    EXPECT_CALL(*endInterface, Copy()).WillRepeatedly(Return(endInterface));

    std::shared_ptr<Sophos::Journal::ViewInterface::EntryInterface> sharedPtrToEntryInterfaceMock = mockJournalEntryInterface;
    EXPECT_CALL(*beginImplInterface, Dereference()).WillRepeatedly(ReturnRef(sharedPtrToEntryInterfaceMock));
    MockJournalViewInterface::ConstIterator journalIterator(beginImplInterface);

    EXPECT_CALL(*beginImplInterface, PrefixIncrement())
        .WillRepeatedly(ReturnRef(*beginImplInterface));

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
    EXPECT_EQ(results.size(), 2);
    EXPECT_TRUE(moreAvailable);

    Mock::VerifyAndClearExpectations(beginImplInterface.get());
    Mock::VerifyAndClearExpectations(endInterface.get());
}