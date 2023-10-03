/***********************************************************************************************

Copyright 2021-2021 Sophos Limited. All rights reserved.

***********************************************************************************************/

#include "SampleJournal.h"

#include "EventJournal/EventJournalWriter.h"

#include "Common/FileSystem/IFilePermissions.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/UtilityImpl/StringUtils.h"
#ifdef SPL_BAZEL
#include "base/tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "base/tests/Common/Helpers/LogInitializedTests.h"
#include "base/tests/Common/Helpers/MockFilePermissions.h"
#include "base/tests/Common/Helpers/MockFileSystem.h"
#include "base/tests/Common/Helpers/TempDir.h"
#else
#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/LogInitializedTests.h"
#include "Common/Helpers/MockFilePermissions.h"
#include "Common/Helpers/MockFileSystem.h"
#include "Common/Helpers/TempDir.h"
#endif

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fstream>
#include <memory>

#include <unistd.h>

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

    void TearDown() override
    {
        Tests::restoreFileSystem();
        Tests::restoreFilePermissions();
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

    void CheckJournalHeader(const EventJournal::Header& header, size_t expected_size, uint32_t expected_sjrn_length)
    {
        EXPECT_STREQ(PRODUCER.c_str(), header.producer.c_str());
        EXPECT_STREQ(SUBJECT.c_str(), header.subject.c_str());
        EXPECT_STREQ("CapnProto", header.serialisationMethod.c_str());
#ifdef SPL_BAZEL
        EXPECT_STREQ("1.0.1", header.serialisationVersion.c_str());
#else
        EXPECT_STREQ("0.8.0", header.serialisationVersion.c_str());
#endif
        EXPECT_EQ(expected_size-8, header.riffLength);
        EXPECT_EQ(expected_sjrn_length, header.sjrnLength);
        EXPECT_EQ(1, header.sjrnVersion);
    }

    void CheckActiveFilename(const std::string& filename, uint64_t expected_producer_id)
    {
        EXPECT_TRUE(Common::UtilityImpl::StringUtils::startswith(filename, SUBJECT));
        EXPECT_TRUE(Common::UtilityImpl::StringUtils::endswith(filename, ".bin"));

        auto strings = Common::UtilityImpl::StringUtils::splitString(filename, "-");
        ASSERT_EQ(3, strings.size());
        EXPECT_STREQ(SUBJECT.c_str(), strings[0].c_str());
        EXPECT_EQ(expected_producer_id, std::stoul(strings[1], nullptr, 16));
    }

    void CheckClosedFilename(const std::string& filename, uint64_t expected_first_producer_id, uint64_t expected_last_producer_id)
    {
        EXPECT_TRUE(Common::UtilityImpl::StringUtils::startswith(filename, SUBJECT));
        EXPECT_TRUE(Common::UtilityImpl::StringUtils::endswith(filename, ".bin"));

        auto strings = Common::UtilityImpl::StringUtils::splitString(filename, "-");
        ASSERT_EQ(5, strings.size());
        EXPECT_STREQ(SUBJECT.c_str(), strings[0].c_str());
        EXPECT_EQ(expected_first_producer_id, std::stoul(strings[1], nullptr, 16));
        EXPECT_EQ(expected_last_producer_id, std::stoul(strings[2], nullptr, 16));
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

    void installJournalFile(const std::string& filename, const uint8_t* data, size_t length)
    {
        auto directory = Common::FileSystem::join(m_journalDir->dirPath(), PRODUCER, SUBJECT);
        if (!Common::FileSystem::fileSystem()->isDirectory(directory))
        {
            Common::FileSystem::fileSystem()->makedirs(directory);

            mode_t directoryPermissions = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP;
            Common::FileSystem::filePermissions()->chmod(m_journalDir->dirPath(), directoryPermissions);
            Common::FileSystem::filePermissions()->chmod(Common::FileSystem::join(m_journalDir->dirPath(), PRODUCER), directoryPermissions);
            Common::FileSystem::filePermissions()->chmod(directory, directoryPermissions);
        }

        auto path = Common::FileSystem::join(directory, filename);
        std::ofstream f(path, std::ios::binary | std::ios::out);
        if (data && length)
        {
            f.write(reinterpret_cast<const char*>(data), length);
        }
        Common::FileSystem::filePermissions()->chmod(path, S_IRUSR | S_IWUSR | S_IRGRP);
    }

    inline static const std::string JOURNAL_LOCATION = "test-journal";
    inline static const std::string PRODUCER = "EventJournalTest";
    inline static const std::string SUBJECT = "Detections";

    static const uint32_t EXPECTED_SJRN_LENGTH = 52;

    std::unique_ptr<EventJournal::Writer> m_writer;
    std::unique_ptr<Tests::TempDir> m_journalDir;
    std::vector<uint8_t> m_eventData;
};

TEST_F(TestEventJournalWriter, InsertDetectionsEvent) // NOLINT
{
    const uint32_t expectedSize = 224;

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
    CheckActiveFilename(filename, 1);
    CheckJournalFile(files.front(), expectedSize, S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP);

    EventJournal::FileInfo info;
    ASSERT_TRUE(m_writer->readFileInfo(files.front(), info));
    CheckJournalHeader(info.header, expectedSize, EXPECTED_SJRN_LENGTH);
    EXPECT_EQ(1, info.firstProducerID);
    EXPECT_NE(0, info.firstTimestamp);
    EXPECT_EQ(1, info.lastProducerID);
    EXPECT_NE(0, info.lastTimestamp);
    EXPECT_EQ(1, info.numEvents);
    EXPECT_EQ(expectedSize, info.size);
    EXPECT_FALSE(info.riffLengthMismatch);
    EXPECT_FALSE(info.truncated);
    EXPECT_FALSE(info.anyLengthErrors);
}

TEST_F(TestEventJournalWriter, AppendDetectionsEvent) // NOLINT
{
    const uint32_t expectedSize = 376;

    m_writer->insert(EventJournal::Subject::Detections, m_eventData);
    m_writer.reset(new EventJournal::Writer(m_journalDir->dirPath(), PRODUCER));
    m_writer->insert(EventJournal::Subject::Detections, m_eventData);

    auto files = Common::FileSystem::fileSystem()->listFiles(
        Common::FileSystem::join(m_journalDir->dirPath(), PRODUCER, SUBJECT));
    ASSERT_EQ(1, files.size());
    auto filename = Common::FileSystem::basename(files.front());
    CheckActiveFilename(filename, 1);
    CheckJournalFile(files.front(), expectedSize, S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP);

    EventJournal::FileInfo info;
    ASSERT_TRUE(m_writer->readFileInfo(files.front(), info));
    CheckJournalHeader(info.header, expectedSize, EXPECTED_SJRN_LENGTH);
    EXPECT_EQ(1, info.firstProducerID);
    EXPECT_NE(0, info.firstTimestamp);
    EXPECT_EQ(2, info.lastProducerID);
    EXPECT_NE(0, info.lastTimestamp);
    EXPECT_EQ(2, info.numEvents);
    EXPECT_EQ(expectedSize, info.size);
    EXPECT_FALSE(info.riffLengthMismatch);
    EXPECT_FALSE(info.truncated);
    EXPECT_FALSE(info.anyLengthErrors);
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

TEST_F(TestEventJournalWriter, InsertDataAboveFileLimitClosesOldFileAndCreatesNewFile) // NOLINT
{
    installJournalFile(journal_detections_bin_multiple_active_filename, journal_detections_bin_multiple, sizeof(journal_detections_bin_multiple));
    m_writer.reset(new EventJournal::Writer(m_journalDir->dirPath(), PRODUCER));

    auto directory = Common::FileSystem::join(m_journalDir->dirPath(), PRODUCER, SUBJECT);
    auto filenameOpen = Common::FileSystem::join(directory, journal_detections_bin_multiple_active_filename);
    auto filenameClosed = Common::FileSystem::join(directory, journal_detections_bin_multiple_closed_filename);
    auto filenameClosedAndCompressed = Common::FileSystem::join(directory, journal_detections_bin_multiple_closed_and_compressed_filename);
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{ mockFileSystem });
    auto mockFilePermissions = new StrictMock<MockFilePermissions>();
    std::unique_ptr<MockFilePermissions> mockIFilePermissionsPtr =
        std::unique_ptr<MockFilePermissions>(mockFilePermissions);
    Tests::replaceFilePermissions(std::move(mockIFilePermissionsPtr));

    EXPECT_CALL(*mockFileSystem, exists(directory)).WillRepeatedly(Return(true));
    std::string detectionsFileNamePrefix("Detections-0000000000000001");
    std::string closedFilePrefix("Detections-0000000000000001-0000000000000003");
    EXPECT_CALL(*mockFileSystem, exists(HasSubstr(Common::FileSystem::join(directory, detectionsFileNamePrefix)))).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, isDirectory(directory)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, listFiles(directory)).WillOnce(Return(std::vector{ filenameClosedAndCompressed, filenameClosed, filenameOpen }));
    EXPECT_CALL(*mockFileSystem, fileSize(filenameOpen)).WillRepeatedly(Return(99804296));
    // moved called when closing file, check to see if the move call contains file with the correct prefix.
    EXPECT_CALL(*mockFileSystem, moveFile(HasSubstr(detectionsFileNamePrefix), HasSubstr(closedFilePrefix))).Times(1);
    EXPECT_CALL(*mockFileSystem, exists(HasSubstr(Common::FileSystem::join(directory, "Detections-0000000000000004")))).WillOnce(Return(true));
    EXPECT_CALL(*mockFilePermissions, chmod(_,_)).Times(1);

    std::vector<uint8_t> data(195 * 1024);
    EXPECT_NO_THROW(m_writer->insert(EventJournal::Subject::Detections, data));
}

TEST_F(TestEventJournalWriter, InsertEventAfterCompressedFileReadsLastProducerID) // NOLINT
{
    std::vector<uint8_t> data{ 0xef, 0xbe, 0xad, 0xde };
    installJournalFile(SUBJECT + "-000000000000000a-000000000000000f-132444736000000000-132444736000000000.xz", data.data(), data.size());
    m_writer.reset(new EventJournal::Writer(m_journalDir->dirPath(), PRODUCER));

    m_writer->insert(EventJournal::Subject::Detections, m_eventData);

    auto directory = Common::FileSystem::join(m_journalDir->dirPath(), PRODUCER, SUBJECT);
    auto files = Common::FileSystem::fileSystem()->listFiles(directory);
    ASSERT_EQ(2, files.size());
    std::string compressedFilename;
    std::string activeFilename;
    for (const auto& file : files)
    {
        if (Common::UtilityImpl::StringUtils::endswith(file, ".xz"))
        {
            compressedFilename = Common::FileSystem::basename(file);
        }
        else if (Common::UtilityImpl::StringUtils::endswith(file, ".bin"))
        {
            activeFilename = Common::FileSystem::basename(file);
        }
        else
        {
            FAIL();
        }
    }
    ASSERT_FALSE(compressedFilename.empty());
    ASSERT_FALSE(activeFilename.empty());
    CheckActiveFilename(activeFilename, 0x10);
    CheckJournalFile(Common::FileSystem::join(directory, activeFilename), 224, S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP);
}

TEST_F(TestEventJournalWriter, InsertEventAfterInvalidFile) // NOLINT
{
    const uint32_t expectedSize = 224;

    std::vector<uint8_t> data{ 0xef, 0xbe, 0xad, 0xde };
    installJournalFile(journal_detections_bin_multiple_active_filename, data.data(), data.size());

    m_writer->insert(EventJournal::Subject::Detections, m_eventData);

    auto directory = Common::FileSystem::join(m_journalDir->dirPath(), PRODUCER, SUBJECT);
    auto files = Common::FileSystem::fileSystem()->listFiles(directory);
    ASSERT_EQ(1, files.size());
    auto filename = Common::FileSystem::basename(files.front());
    EXPECT_STRNE(journal_detections_bin_multiple_active_filename.c_str(), filename.c_str());
    CheckActiveFilename(filename, 1);
    CheckJournalFile(files.front(), expectedSize, S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP);
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

TEST_F(TestEventJournalWriter, ReadFileInfoDetectionsFile) // NOLINT
{
    installJournalFile(journal_detections_bin_multiple_active_filename, journal_detections_bin_multiple, sizeof(journal_detections_bin_multiple));

    auto directory = Common::FileSystem::join(m_journalDir->dirPath(), PRODUCER, SUBJECT);
    auto files = Common::FileSystem::fileSystem()->listFiles(directory);
    ASSERT_EQ(1, files.size());

    EventJournal::FileInfo info;
    ASSERT_TRUE(m_writer->readFileInfo(files.front(), info));
    CheckJournalHeader(info.header, sizeof(journal_detections_bin_multiple), EXPECTED_SJRN_LENGTH);
    EXPECT_EQ(1, info.firstProducerID);
    EXPECT_EQ(132729637080000000, info.firstTimestamp);
    EXPECT_EQ(3, info.lastProducerID);
    EXPECT_EQ(132729637110000000, info.lastTimestamp);
    EXPECT_EQ(3, info.numEvents);
    EXPECT_EQ(sizeof(journal_detections_bin_multiple), info.size);
    EXPECT_FALSE(info.riffLengthMismatch);
    EXPECT_FALSE(info.truncated);
    EXPECT_FALSE(info.anyLengthErrors);
}

TEST_F(TestEventJournalWriter, ReadFileInfoRIFFLengthMismatch) // NOLINT
{
    const uint32_t length = 368;

    std::vector<uint8_t> data(journal_detections_bin_multiple, journal_detections_bin_multiple+sizeof(journal_detections_bin_multiple));
    memcpy(&data[4], &length, sizeof(length));

    installJournalFile(journal_detections_bin_multiple_active_filename, data.data(), data.size());

    auto directory = Common::FileSystem::join(m_journalDir->dirPath(), PRODUCER, SUBJECT);
    auto files = Common::FileSystem::fileSystem()->listFiles(directory);
    ASSERT_EQ(1, files.size());

    EventJournal::FileInfo info;
    ASSERT_TRUE(m_writer->readFileInfo(files.front(), info));
    CheckJournalHeader(info.header, length+8, EXPECTED_SJRN_LENGTH);
    EXPECT_EQ(1, info.firstProducerID);
    EXPECT_EQ(132729637080000000, info.firstTimestamp);
    EXPECT_EQ(3, info.lastProducerID);
    EXPECT_EQ(132729637110000000, info.lastTimestamp);
    EXPECT_EQ(3, info.numEvents);
    EXPECT_EQ(sizeof(journal_detections_bin_multiple), info.size);
    EXPECT_TRUE(info.riffLengthMismatch);
    EXPECT_FALSE(info.truncated);
    EXPECT_TRUE(info.anyLengthErrors);
}

TEST_F(TestEventJournalWriter, ReadFileInfoTruncatedFile) // NOLINT
{
    installJournalFile(journal_detections_bin_multiple_active_filename, journal_detections_bin_multiple, sizeof(journal_detections_bin_multiple)-13);

    auto directory = Common::FileSystem::join(m_journalDir->dirPath(), PRODUCER, SUBJECT);
    auto files = Common::FileSystem::fileSystem()->listFiles(directory);
    ASSERT_EQ(1, files.size());

    EventJournal::FileInfo info;
    ASSERT_TRUE(m_writer->readFileInfo(files.front(), info));
    CheckJournalHeader(info.header, sizeof(journal_detections_bin_multiple), EXPECTED_SJRN_LENGTH);
    EXPECT_EQ(1, info.firstProducerID);
    EXPECT_EQ(132729637080000000, info.firstTimestamp);
    EXPECT_EQ(2, info.lastProducerID);
    EXPECT_EQ(132729637100000000, info.lastTimestamp);
    EXPECT_EQ(2, info.numEvents);
    EXPECT_EQ(sizeof(journal_detections_bin_multiple)-13, info.size);
    EXPECT_TRUE(info.riffLengthMismatch);
    EXPECT_TRUE(info.truncated);
    EXPECT_TRUE(info.anyLengthErrors);
}

TEST_F(TestEventJournalWriter, ReadFileInfoMissingFile) // NOLINT
{
    auto directory = Common::FileSystem::join(m_journalDir->dirPath(), PRODUCER, SUBJECT);
    auto filename = Common::FileSystem::join(directory, "invalid.bin");

    EventJournal::FileInfo info;
    EXPECT_FALSE(m_writer->readFileInfo(filename, info));
}

TEST_F(TestEventJournalWriter, ReadFileInfoEmptyFile) // NOLINT
{
    installJournalFile("invalid.bin", nullptr, 0);

    auto directory = Common::FileSystem::join(m_journalDir->dirPath(), PRODUCER, SUBJECT);
    auto files = Common::FileSystem::fileSystem()->listFiles(directory);
    ASSERT_EQ(1, files.size());

    EventJournal::FileInfo info;
    EXPECT_FALSE(m_writer->readFileInfo(files.front(), info));
}

TEST_F(TestEventJournalWriter, ReadFileInfoInvalidRIFFFile) // NOLINT
{
    std::vector<uint8_t> data{ 0xef, 0xbe, 0xad, 0xde };
    data.resize(22);

    installJournalFile("invalid.bin", data.data(), data.size());

    auto directory = Common::FileSystem::join(m_journalDir->dirPath(), PRODUCER, SUBJECT);
    auto files = Common::FileSystem::fileSystem()->listFiles(directory);
    ASSERT_EQ(1, files.size());

    EventJournal::FileInfo info;
    EXPECT_FALSE(m_writer->readFileInfo(files.front(), info));
}

TEST_F(TestEventJournalWriter, ReadFileInfoInvalidSJRNFile) // NOLINT
{
    std::vector<uint8_t> data{ 0x52, 0x49, 0x46, 0x46, 0x00, 0x00, 0x00, 0x00, 0xef, 0xbe, 0xad, 0xde };
    data.resize(22);

    installJournalFile("invalid.bin", data.data(), data.size());

    auto directory = Common::FileSystem::join(m_journalDir->dirPath(), PRODUCER, SUBJECT);
    auto files = Common::FileSystem::fileSystem()->listFiles(directory);
    ASSERT_EQ(1, files.size());

    EventJournal::FileInfo info;
    EXPECT_FALSE(m_writer->readFileInfo(files.front(), info));
}

TEST_F(TestEventJournalWriter, ReadFileInfoInvalidHDRFile) // NOLINT
{
    std::vector<uint8_t> data{ 0x52, 0x49, 0x46, 0x46, 0x00, 0x00, 0x00, 0x00,
                               0x73, 0x6a, 0x72, 0x6e, 0xef, 0xbe, 0xad, 0xde };
    data.resize(22);

    installJournalFile("invalid.bin", data.data(), data.size());

    auto directory = Common::FileSystem::join(m_journalDir->dirPath(), PRODUCER, SUBJECT);
    auto files = Common::FileSystem::fileSystem()->listFiles(directory);
    ASSERT_EQ(1, files.size());

    EventJournal::FileInfo info;
    EXPECT_FALSE(m_writer->readFileInfo(files.front(), info));
}

TEST_F(TestEventJournalWriter, ReadFileInfoInvalidSJRNLength) // NOLINT
{
    std::vector<uint8_t> data{ 0x52, 0x49, 0x46, 0x46, 0x00, 0x00, 0x00, 0x00, 0x73, 0x6a,
                               0x72, 0x6e, 0x68, 0x64, 0x72, 0x20, 0xff, 0x00, 0x00, 0x00 };
    data.resize(22);

    installJournalFile("invalid.bin", data.data(), data.size());

    auto directory = Common::FileSystem::join(m_journalDir->dirPath(), PRODUCER, SUBJECT);
    auto files = Common::FileSystem::fileSystem()->listFiles(directory);
    ASSERT_EQ(1, files.size());

    EventJournal::FileInfo info;
    EXPECT_FALSE(m_writer->readFileInfo(files.front(), info));
}

TEST_F(TestEventJournalWriter, ReadFileInfoTooLargeSJRNLength) // NOLINT
{
    std::vector<uint8_t> data{
        0x52, 0x49, 0x46, 0x46, 0x00, 0x00, 0x00, 0x00, 0x73, 0x6a, 0x72, 0x6e, 0x68, 0x64, 0x72,
        0x20, 0xff, 0x0f, 0x00, 0x00, 0x01, 0x00, 0x45, 0x00, 0x45, 0x00, 0x45, 0x00, 0x45, 0x00
    };
    data.resize(6 * 1024);

    installJournalFile("invalid.bin", data.data(), data.size());

    auto directory = Common::FileSystem::join(m_journalDir->dirPath(), PRODUCER, SUBJECT);
    auto files = Common::FileSystem::fileSystem()->listFiles(directory);
    ASSERT_EQ(1, files.size());

    EventJournal::FileInfo info;
    EXPECT_FALSE(m_writer->readFileInfo(files.front(), info));
}

TEST_F(TestEventJournalWriter, NewSerialisationMethodClosesFile) // NOLINT
{
    std::vector<uint8_t> data(journal_detections_bin_multiple, journal_detections_bin_multiple+sizeof(journal_detections_bin_multiple));
    data[50] = 'K';

    installJournalFile(journal_detections_bin_multiple_active_filename, data.data(), data.size());
    m_writer.reset(new EventJournal::Writer(m_journalDir->dirPath(), PRODUCER));

    auto directory = Common::FileSystem::join(m_journalDir->dirPath(), PRODUCER, SUBJECT);
    auto files = Common::FileSystem::fileSystem()->listFiles(directory);
    ASSERT_EQ(1, files.size());
    auto filename = Common::FileSystem::basename(files.front());
    EXPECT_STREQ(journal_detections_bin_multiple_closed_filename.c_str(), filename.c_str());
    CheckJournalFile(files.front(), data.size(), S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP);
}

TEST_F(TestEventJournalWriter, NewSerialisationVersionClosesFile) // NOLINT
{
    std::vector<uint8_t> data(journal_detections_bin_multiple, journal_detections_bin_multiple+sizeof(journal_detections_bin_multiple));
    data[62] = '7';

    installJournalFile(journal_detections_bin_multiple_active_filename, data.data(), data.size());
    m_writer.reset(new EventJournal::Writer(m_journalDir->dirPath(), PRODUCER));

    auto directory = Common::FileSystem::join(m_journalDir->dirPath(), PRODUCER, SUBJECT);
    auto files = Common::FileSystem::fileSystem()->listFiles(directory);
    ASSERT_EQ(1, files.size());
    auto filename = Common::FileSystem::basename(files.front());
    EXPECT_STREQ(journal_detections_bin_multiple_closed_filename.c_str(), filename.c_str());
    CheckJournalFile(files.front(), data.size(), S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP);
}

TEST_F(TestEventJournalWriter, NewSJRNVersionClosesFile) // NOLINT
{
    const uint16_t version = 0;

    std::vector<uint8_t> data(journal_detections_bin_multiple, journal_detections_bin_multiple+sizeof(journal_detections_bin_multiple));
    memcpy(&data[20], &version, sizeof(version));

    installJournalFile(journal_detections_bin_multiple_active_filename, data.data(), data.size());
    m_writer.reset(new EventJournal::Writer(m_journalDir->dirPath(), PRODUCER));

    auto directory = Common::FileSystem::join(m_journalDir->dirPath(), PRODUCER, SUBJECT);
    auto files = Common::FileSystem::fileSystem()->listFiles(directory);
    ASSERT_EQ(1, files.size());
    auto filename = Common::FileSystem::basename(files.front());
    EXPECT_STREQ(journal_detections_bin_multiple_closed_filename.c_str(), filename.c_str());
    CheckJournalFile(files.front(), data.size(), S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP);
}

TEST_F(TestEventJournalWriter, PruneTruncatedEventsOnStartUp) // NOLINT
{
    const std::string closedFilename = "Detections-0000000000000001-0000000000000002-132729637080000000-132729637100000000.bin";

    installJournalFile(journal_detections_bin_multiple_active_filename, journal_detections_bin_multiple, sizeof(journal_detections_bin_multiple)-13);
    m_writer.reset(new EventJournal::Writer(m_journalDir->dirPath(), PRODUCER));

    auto directory = Common::FileSystem::join(m_journalDir->dirPath(), PRODUCER, SUBJECT);
    auto files = Common::FileSystem::fileSystem()->listFiles(directory);
    ASSERT_EQ(1, files.size());
    auto filename = Common::FileSystem::basename(files.front());
    EXPECT_STREQ(closedFilename.c_str(), filename.c_str());

    m_writer->pruneTruncatedEvents(files.front());
    CheckJournalFile(files.front(), 376, S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP);
}

TEST_F(TestEventJournalWriter, PruneTruncatedEventsOnInsert) // NOLINT
{
    const int numEvents = 3;
    const int expectedSize = 528;

    for (int i = 0; i < numEvents; i++)
    {
        m_writer->insert(EventJournal::Subject::Detections, m_eventData);
    }

    auto directory = Common::FileSystem::join(m_journalDir->dirPath(), PRODUCER, SUBJECT);
    auto files = Common::FileSystem::fileSystem()->listFiles(directory);
    ASSERT_EQ(1, files.size());
    auto filename = Common::FileSystem::basename(files.front());
    CheckActiveFilename(Common::FileSystem::basename(filename), 1);
    CheckJournalFile(files.front(), expectedSize, S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP);

    ASSERT_EQ(0, truncate(files.front().c_str(), expectedSize-13));

    m_writer->insert(EventJournal::Subject::Detections, m_eventData);

    files = Common::FileSystem::fileSystem()->listFiles(directory);
    ASSERT_EQ(2, files.size());
    std::string activeFilename;
    std::string closedFilename;
    for (const auto& file : files)
    {
        auto filename = Common::FileSystem::basename(file);
        if (Common::UtilityImpl::StringUtils::splitString(filename, "-").size() == 5)
        {
            closedFilename = filename;
        }
        else
        {
            activeFilename = filename;
        }
    }
    ASSERT_FALSE(activeFilename.empty());
    ASSERT_FALSE(closedFilename.empty());
    CheckClosedFilename(closedFilename, 1, 2);
    CheckActiveFilename(activeFilename, 4);

    CheckJournalFile(Common::FileSystem::join(directory, activeFilename), 224, S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP);

    m_writer->pruneTruncatedEvents(Common::FileSystem::join(directory, closedFilename));
    CheckJournalFile(Common::FileSystem::join(directory, closedFilename), 376, S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP);
}

TEST_F(TestEventJournalWriter, PruneTruncatedEventsOnRIFFLengthMismatch) // NOLINT
{
    const uint32_t length = 368;

    std::vector<uint8_t> data(journal_detections_bin_multiple, journal_detections_bin_multiple+sizeof(journal_detections_bin_multiple));
    memcpy(&data[4], &length, sizeof(length));

    installJournalFile(journal_detections_bin_multiple_active_filename, data.data(), data.size());
    m_writer.reset(new EventJournal::Writer(m_journalDir->dirPath(), PRODUCER));

    auto directory = Common::FileSystem::join(m_journalDir->dirPath(), PRODUCER, SUBJECT);
    auto files = Common::FileSystem::fileSystem()->listFiles(directory);
    ASSERT_EQ(1, files.size());
    auto filename = Common::FileSystem::basename(files.front());
    EXPECT_STREQ(journal_detections_bin_multiple_closed_filename.c_str(), filename.c_str());

    m_writer->pruneTruncatedEvents(files.front());
    CheckJournalFile(files.front(), data.size(), S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP);
}

TEST_F(TestEventJournalWriter, RemoveFileWithNoCompleteEventsOnStartUp) // NOLINT
{
    const uint32_t expectedSize = 224;

    m_writer->insert(EventJournal::Subject::Detections, m_eventData);

    auto directory = Common::FileSystem::join(m_journalDir->dirPath(), PRODUCER, SUBJECT);
    auto files = Common::FileSystem::fileSystem()->listFiles(directory);
    ASSERT_EQ(1, files.size());
    auto filename = Common::FileSystem::basename(files.front());
    CheckActiveFilename(Common::FileSystem::basename(filename), 1);
    CheckJournalFile(files.front(), expectedSize, S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP);

    ASSERT_EQ(0, truncate(files.front().c_str(), expectedSize-13));

    m_writer.reset(new EventJournal::Writer(m_journalDir->dirPath(), PRODUCER));
    EXPECT_TRUE(Common::FileSystem::fileSystem()->listFiles(directory).empty());
}

TEST_F(TestEventJournalWriter, RemoveFileWithNoCompleteEventsOnInsert) // NOLINT
{
    const uint32_t expectedSize = 224;

    m_writer->insert(EventJournal::Subject::Detections, m_eventData);

    auto directory = Common::FileSystem::join(m_journalDir->dirPath(), PRODUCER, SUBJECT);
    auto files = Common::FileSystem::fileSystem()->listFiles(directory);
    ASSERT_EQ(1, files.size());
    auto filename = Common::FileSystem::basename(files.front());
    CheckActiveFilename(Common::FileSystem::basename(filename), 1);
    CheckJournalFile(files.front(), expectedSize, S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP);

    ASSERT_EQ(0, truncate(files.front().c_str(), expectedSize-13));

    m_writer->insert(EventJournal::Subject::Detections, m_eventData);

    files = Common::FileSystem::fileSystem()->listFiles(directory);
    ASSERT_EQ(1, files.size());
    filename = Common::FileSystem::basename(files.front());
    CheckActiveFilename(Common::FileSystem::basename(filename), 2);
    CheckJournalFile(files.front(), expectedSize, S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP);

    EventJournal::FileInfo info;
    ASSERT_TRUE(m_writer->readFileInfo(files.front(), info));
    CheckJournalHeader(info.header, expectedSize, EXPECTED_SJRN_LENGTH);
    EXPECT_EQ(2, info.firstProducerID);
    EXPECT_NE(0, info.firstTimestamp);
    EXPECT_EQ(2, info.lastProducerID);
    EXPECT_NE(0, info.lastTimestamp);
    EXPECT_EQ(1, info.numEvents);
    EXPECT_EQ(expectedSize, info.size);
    EXPECT_FALSE(info.riffLengthMismatch);
    EXPECT_FALSE(info.truncated);
    EXPECT_FALSE(info.anyLengthErrors);
}

TEST_F(TestEventJournalWriter, testGetExistingFileReturnsEmptyStringWhenNoFiles) // NOLINT
{
    auto directory = Common::FileSystem::join(m_journalDir->dirPath(), PRODUCER, SUBJECT);
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{ mockFileSystem });
    EXPECT_CALL(*mockFileSystem, isDirectory(directory)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, listFiles(directory)).WillOnce(Return(std::vector<std::string>{}));

    EXPECT_EQ(EventJournal::getOpenFileFromDirectory(m_journalDir->dirPath(),PRODUCER,SUBJECT), "");
}

TEST_F(TestEventJournalWriter, testGetExistingFileReturnsEmptyStringWhenNoOpenFile) // NOLINT
{
    auto directory = Common::FileSystem::join(m_journalDir->dirPath(), PRODUCER, SUBJECT);
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{ mockFileSystem });
    EXPECT_CALL(*mockFileSystem, isDirectory(directory)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, listFiles(directory)).WillOnce(Return(std::vector<std::string>{
        "Detections-0000000000000001-0000000000033b9a-132727414310000000-132729742680000000.xz",
        "Detections-0000000000041413-0000000000041c59-132729797430000000-132729798310000000.bin",
        "Detections-0000000000041c5a-000000000004249f-132729798310000000-132729799230000000.bin"
    }));

    EXPECT_EQ(EventJournal::getOpenFileFromDirectory(m_journalDir->dirPath(),PRODUCER,SUBJECT), "");
}

TEST_F(TestEventJournalWriter, testGetExistingFileReturnsOpenSubjectFile) // NOLINT
{
    auto directory = Common::FileSystem::join(m_journalDir->dirPath(), PRODUCER, SUBJECT);
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{ mockFileSystem });
    EXPECT_CALL(*mockFileSystem, isDirectory(directory)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, listFiles(directory)).WillOnce(Return(std::vector<std::string>{
        "Detections-0000000000000001-0000000000033b9a-132727414310000000-132729742680000000.xz",
        "Detections-0000000000041413-0000000000041c59-132729797430000000-132729798310000000.bin",
        "Detections-0000000000041c5a-000000000004249f-132729798310000000-132729799230000000.bin",
        "Detections-0000000000042ce7-132729800360000000.bin"
    }));

    EXPECT_EQ(EventJournal::getOpenFileFromDirectory(m_journalDir->dirPath(),PRODUCER,SUBJECT), "Detections-0000000000042ce7-132729800360000000.bin");
}

TEST_F(TestEventJournalWriter, testGetExistingFileReturnsCorrectOpenSubjectFileWhenMultipleSubjectsHaveOpenFiles) // NOLINT
{
    auto directory = Common::FileSystem::join(m_journalDir->dirPath(), PRODUCER, SUBJECT);
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{ mockFileSystem });
    EXPECT_CALL(*mockFileSystem, isDirectory(directory)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, listFiles(directory)).WillOnce(Return(std::vector<std::string>{
        "OtherSubject-0000000000042ce7-132729800360000000.bin",
        "Detections-0000000000042ce7-132729800360000000.bin"
    }));

    EXPECT_EQ(EventJournal::getOpenFileFromDirectory(m_journalDir->dirPath(),PRODUCER,SUBJECT), "Detections-0000000000042ce7-132729800360000000.bin");
}

TEST_F(TestEventJournalWriter, testIsOpenSubjectFileOnlyReturnsTrueForOpenFiles) // NOLINT
{
    EXPECT_TRUE(EventJournal::isOpenSubjectFile("Detections", "Detections-0000000000042ce7-132729800360000000.bin"));

    // compressed + closed files
    EXPECT_FALSE(EventJournal::isOpenSubjectFile("Detections", "Detections-0000000000000001-0000000000033b9a-132727414310000000-132729742680000000.xz"));
    EXPECT_FALSE(EventJournal::isOpenSubjectFile("Detections", "Detections-0000000000041413-0000000000041c59-132729797430000000-132729798310000000.bin"));
    EXPECT_FALSE(EventJournal::isOpenSubjectFile("Detections", "Detections-0000000000041c5a-000000000004249f-132729798310000000-132729799230000000.bin"));

    // capital hex
    EXPECT_FALSE(EventJournal::isOpenSubjectFile("Detections", "Detections-0000000000042CE7-132729800360000000.bin"));
    // check escape on decimal point for extension
    EXPECT_FALSE(EventJournal::isOpenSubjectFile("Detections", "Detections-0000000000042ce7-132729800360000000dbin"));
    // no .bin
    EXPECT_FALSE(EventJournal::isOpenSubjectFile("Detections", "Detections-0000000000042ce7-132729800360000000.xz"));
    // wrong extension
    EXPECT_FALSE(EventJournal::isOpenSubjectFile("Detections", "Detections-000000000000000042ce7-132729800360000000.bin"));
    // no uuids
    EXPECT_FALSE(EventJournal::isOpenSubjectFile("Detections", "Detections--.bin"));
}