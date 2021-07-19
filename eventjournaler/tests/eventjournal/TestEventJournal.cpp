/***********************************************************************************************

Copyright 2021-2021 Sophos Limited. All rights reserved.

***********************************************************************************************/

#include <Common/FileSystem/IFilePermissions.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/Helpers/FileSystemReplaceAndRestore.h>
#include <Common/Helpers/LogInitializedTests.h>
#include <Common/Helpers/MockFileSystem.h>
#include <Common/Helpers/TempDir.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <EventJournal/EventJournalWriter.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fstream>
#include <memory>

class TestEventJournalWriter : public LogOffInitializedTests
{
protected:
    void SetUp() override
    {
        m_journalDir = Tests::TempDir::makeTempDir(JOURNAL_LOCATION);

        m_writer.reset(new EventJournal::Writer(m_journalDir->dirPath(), PRODUCER));

        EventJournal::Detection detection;
        detection.subType = "Test";
        detection.data = R"({"threatName":"EICAR-AV-Test","threatPath":"/home/admin/eicar.com"})";

        m_eventData = EventJournal::encode(detection);
        EXPECT_FALSE(m_eventData.empty());
    }

    void CheckJournalFile(const std::string& filename, size_t expected_size, mode_t expected_mode)
    {
        EXPECT_EQ(expected_size, Common::FileSystem::fileSystem()->fileSize(filename));
        EXPECT_EQ(expected_mode, Common::FileSystem::filePermissions()->getFilePermissions(filename));

        std::ifstream f(filename, std::ios::binary | std::ios::in);
        std::vector<uint8_t> buffer(16);

        f.read(reinterpret_cast<char*>(&buffer[0]), 16);

        u_int32_t riff = 0;
        u_int32_t length = 0;
        u_int32_t sjrn = 0;
        u_int32_t hdr = 0;

        memcpy(&riff, &buffer[0], sizeof(riff));
        memcpy(&length, &buffer[4], sizeof(length));
        memcpy(&sjrn, &buffer[8], sizeof(sjrn));
        memcpy(&hdr, &buffer[12], sizeof(hdr));

        EXPECT_EQ(0x46464952, riff);
        EXPECT_EQ(expected_size - 8, length);
        EXPECT_EQ(0x6e726a73, sjrn);
        EXPECT_EQ(0x20726468, hdr);
    }

    mode_t getDirectoryPermissions(const std::string& directory)
    {
        struct stat buffer{};
        int result = stat(directory.c_str(), &buffer);

        if (result != 0)
        {
            throw std::runtime_error("stat failed");
        }

        if (!S_ISDIR(buffer.st_mode))
        {
            throw std::runtime_error("not a directory");
        }

        return buffer.st_mode;
    }

    inline static const std::string JOURNAL_LOCATION = "test-journal";
    inline static const std::string PRODUCER = "EventJournalTest";
    inline static const std::string SUBJECT = "Detections";

    std::unique_ptr<EventJournal::Writer> m_writer;
    std::unique_ptr<Tests::TempDir> m_journalDir;
    std::vector<uint8_t> m_eventData;
};

TEST_F(TestEventJournalWriter, InsertDetectionsEvent) // NOLINT
{
    m_writer->insert(EventJournal::Subject::Detections, m_eventData);

    auto directory = Common::FileSystem::join(m_journalDir->dirPath(), PRODUCER, SUBJECT);
    ASSERT_TRUE(Common::FileSystem::fileSystem()->isDirectory(directory));
    mode_t directoryPermissions = S_IFDIR | S_IXUSR | S_IXGRP | S_IRUSR | S_IWUSR | S_IRGRP;
    EXPECT_EQ(directoryPermissions, getDirectoryPermissions(m_journalDir->dirPath()));
    EXPECT_EQ(
        directoryPermissions, getDirectoryPermissions(Common::FileSystem::join(m_journalDir->dirPath(), PRODUCER)));
    EXPECT_EQ(directoryPermissions, getDirectoryPermissions(directory));

    auto files = Common::FileSystem::fileSystem()->listFiles(directory);
    ASSERT_EQ(1, files.size());
    auto filename = Common::FileSystem::basename(files.front());
    EXPECT_TRUE(Common::UtilityImpl::StringUtils::startswith(filename, SUBJECT));
    EXPECT_TRUE(Common::UtilityImpl::StringUtils::endswith(filename, ".bin"));
    CheckJournalFile(files.front(), 224, S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP);
}

TEST_F(TestEventJournalWriter, AppendDetectionsEvent) // NOLINT
{
    m_writer->insert(EventJournal::Subject::Detections, m_eventData);
    m_writer.reset(new EventJournal::Writer(m_journalDir->dirPath(), PRODUCER));
    m_writer->insert(EventJournal::Subject::Detections, m_eventData);

    auto files = Common::FileSystem::fileSystem()->listFiles(
        Common::FileSystem::join(m_journalDir->dirPath(), PRODUCER, SUBJECT));
    ASSERT_EQ(1, files.size());
    auto filename = Common::FileSystem::basename(files.front());
    EXPECT_TRUE(Common::UtilityImpl::StringUtils::startswith(filename, SUBJECT));
    EXPECT_TRUE(Common::UtilityImpl::StringUtils::endswith(filename, ".bin"));
    CheckJournalFile(files.front(), 376, S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP);
}

TEST_F(TestEventJournalWriter, InsertUnsupportedEventThrows) // NOLINT
{
    EXPECT_THROW(m_writer->insert(EventJournal::Subject::NumSubjects, m_eventData), std::runtime_error);

    EXPECT_FALSE(
        Common::FileSystem::fileSystem()->isDirectory(Common::FileSystem::join(m_journalDir->dirPath(), PRODUCER)));
}

TEST_F(TestEventJournalWriter, InsertUnalignedDataThrows) // NOLINT
{
    m_eventData.pop_back();
    EXPECT_THROW(m_writer->insert(EventJournal::Subject::Detections, m_eventData), std::runtime_error);

    EXPECT_FALSE(
        Common::FileSystem::fileSystem()->isDirectory(Common::FileSystem::join(m_journalDir->dirPath(), PRODUCER)));
}

TEST_F(TestEventJournalWriter, InsertTooLargeDataThrows) // NOLINT
{
    std::vector<uint8_t> data(196 * 1024);
    EXPECT_THROW(m_writer->insert(EventJournal::Subject::Detections, data), std::runtime_error);

    EXPECT_FALSE(
        Common::FileSystem::fileSystem()->isDirectory(Common::FileSystem::join(m_journalDir->dirPath(), PRODUCER)));
}

TEST_F(TestEventJournalWriter, InsertDataAboveFileLimitThrows) // NOLINT
{
    auto directory = Common::FileSystem::join(m_journalDir->dirPath(), PRODUCER, SUBJECT);
    auto filename = Common::FileSystem::join(directory, SUBJECT + "-0000000000000001-132444736000000000.bin");
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{ mockFileSystem });
    EXPECT_CALL(*mockFileSystem, exists(directory)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, exists(Common::FileSystem::join(m_journalDir->dirPath(), PRODUCER)))
        .WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, isDirectory(directory)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, listFiles(directory)).WillOnce(Return(std::vector{ filename }));
    EXPECT_CALL(*mockFileSystem, fileSize(filename)).WillOnce(Return(99804296));

    std::vector<uint8_t> data(195 * 1024);
    EXPECT_THROW(m_writer->insert(EventJournal::Subject::Detections, data), std::runtime_error);
}

TEST_F(TestEventJournalWriter, CreateDirectoryFailureThrows) // NOLINT
{
    auto directory = Common::FileSystem::join(m_journalDir->dirPath(), PRODUCER, SUBJECT);
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{ mockFileSystem });
    EXPECT_CALL(*mockFileSystem, exists(directory)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, makedirs(directory)).WillOnce(Throw(std::runtime_error("Test exception")));

    EXPECT_THROW(m_writer->insert(EventJournal::Subject::Detections, m_eventData), std::runtime_error);
}
