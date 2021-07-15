/***********************************************************************************************

Copyright 2021-2021 Sophos Limited. All rights reserved.

***********************************************************************************************/

#include <fstream>
#include <filesystem>
#include <memory>

#include <eventjournal/EventJournalWriter.h>

#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/UtilityImpl/StringUtils.h>

#include <Common/Helpers/LogInitializedTests.h>
#include <gtest/gtest.h>

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

    void TearDown() override
    {
        fs::remove_all(JOURNAL_LOCATION);
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
    EXPECT_EQ(224, Common::FileSystem::fileSystem()->fileSize(files.front()));
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
