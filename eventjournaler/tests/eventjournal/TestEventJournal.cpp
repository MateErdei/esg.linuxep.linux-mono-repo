/***********************************************************************************************

Copyright 2021-2021 Sophos Limited. All rights reserved.

***********************************************************************************************/

#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/Helpers/FileSystemReplaceAndRestore.h>
#include <Common/Helpers/LogInitializedTests.h>
#include <Common/Helpers/MockFileSystem.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <eventjournal/EventJournalWriter.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <memory>

namespace fs = std::filesystem;

class TestEventJournalWriter : public LogOffInitializedTests
{
protected:
    void SetUp() override
    {
        fs::remove_all(JOURNAL_LOCATION);
        ASSERT_TRUE(fs::create_directories(JOURNAL_LOCATION));

        m_writer.reset(new EventJournal::Writer(JOURNAL_LOCATION, PRODUCER));

        EventJournal::Detection detection;
        detection.subType = "Test";
        detection.data = R"({"threatName":"EICAR-AV-Test","threatPath":"/home/admin/eicar.com"})";

        m_eventData = EventJournal::encode(detection);
        EXPECT_FALSE(m_eventData.empty());
    }

    void TearDown() override { fs::remove_all(JOURNAL_LOCATION); }

    void CheckJournalFile(const std::string& filename, size_t expected_size)
    {
        EXPECT_EQ(expected_size, Common::FileSystem::fileSystem()->fileSize(filename));

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

    inline static const std::string JOURNAL_LOCATION = "/tmp/test-journal";
    inline static const std::string PRODUCER = "EventJournalTest";
    inline static const std::string SUBJECT = "Detections";

    std::unique_ptr<EventJournal::Writer> m_writer;
    std::vector<uint8_t> m_eventData;
};

TEST_F(TestEventJournalWriter, InsertDetectionsEvent) // NOLINT
{
    m_writer->insert(EventJournal::Subject::Detections, m_eventData);

    auto directory = Common::FileSystem::join(JOURNAL_LOCATION, PRODUCER, SUBJECT);
    ASSERT_TRUE(Common::FileSystem::fileSystem()->isDirectory(directory));

    auto files = Common::FileSystem::fileSystem()->listFiles(directory);
    ASSERT_EQ(1, files.size());
    auto filename = Common::FileSystem::basename(files.front());
    EXPECT_TRUE(Common::UtilityImpl::StringUtils::startswith(filename, SUBJECT));
    EXPECT_TRUE(Common::UtilityImpl::StringUtils::endswith(filename, ".bin"));
    CheckJournalFile(files.front(), 224);
}

TEST_F(TestEventJournalWriter, InsertUnsupportedEventThrows) // NOLINT
{
    EXPECT_THROW(m_writer->insert(EventJournal::Subject::NumSubjects, m_eventData), std::runtime_error);

    EXPECT_FALSE(Common::FileSystem::fileSystem()->isDirectory(Common::FileSystem::join(JOURNAL_LOCATION, PRODUCER)));
}

TEST_F(TestEventJournalWriter, InsertUnalignedDataThrows) // NOLINT
{
    m_eventData.pop_back();
    EXPECT_THROW(m_writer->insert(EventJournal::Subject::Detections, m_eventData), std::runtime_error);

    EXPECT_FALSE(Common::FileSystem::fileSystem()->isDirectory(Common::FileSystem::join(JOURNAL_LOCATION, PRODUCER)));
}

TEST_F(TestEventJournalWriter, CreateDirectoryFailureThrows) // NOLINT
{
    auto directory = Common::FileSystem::join(JOURNAL_LOCATION, PRODUCER, SUBJECT);
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{ mockFileSystem });
    EXPECT_CALL(*mockFileSystem, exists(directory)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, makedirs(directory)).WillOnce(Throw(std::runtime_error("Test exception")));

    EXPECT_THROW(m_writer->insert(EventJournal::Subject::Detections, m_eventData), std::runtime_error);
}
