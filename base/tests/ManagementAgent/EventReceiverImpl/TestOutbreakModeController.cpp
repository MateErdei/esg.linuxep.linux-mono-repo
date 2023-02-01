// Copyright 2023 Sophos Limited. All rights reserved.

#define TEST_PUBLIC public

#include "ManagementAgent/EventReceiverImpl/OutbreakModeController.h"

#include "Common/FileSystem/IFileNotFoundException.h"

#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/TestSpecificDirectory.h"
#include "tests/Common/Helpers/MockFileSystem.h"

using namespace ManagementAgent::EventReceiverImpl;
namespace fs = sophos_filesystem;

namespace
{
    const int OUTBREAK_COUNT = 100;

    const std::string DETECTION_XML = R"sophos(<?xml version="1.0" encoding="utf-8"?>
<event type="sophos.core.detection" ts="1970-01-01T00:02:03.000Z">
  <user userId="username"/>
  <alert id="fedcba98-7654-3210-fedc-ba9876543210" name="EICAR-AV-Test" threatType="1" origin="1" remote="true">
    <sha256>131f95c51cc819465fa1797f6ccacf9d494aaaff46fa3eac73ae63ffbdfd8267</sha256>
    <path>/path/to/eicar.txt</path>
  </alert>
</event>)sophos";

    class TestOutbreakModeController : public TestSpecificDirectory
    {
    public:
        TestOutbreakModeController()
            : TestSpecificDirectory("managementagent")
        {}

    protected:
        void SetUp() override
        {
            testDir_ = test_common::createTestSpecificDirectory();
            fs::current_path(testDir_);
            auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
            appConfig.setData(Common::ApplicationConfiguration::SOPHOS_INSTALL, testDir_);
            fs::create_directories(testDir_ / "tmp");
            fs::create_directories(testDir_ / "var" / "sophosspl");
            expectedStatusFile_ = testDir_ / "var" / "sophosspl" / "outbreak_status.json";
        }

        void TearDown() override
        {
            test_common::removeTestSpecificDirectory(testDir_);
        }

        fs::path testDir_;
        Path expectedStatusFile_;

        static void enterOutbreakMode(OutbreakModeControllerPtr& controller)
        {
            for (auto i=0; i<OUTBREAK_COUNT+1; i++)
            {
                controller->recordEventAndDetermineIfItShouldBeDropped("CORE", DETECTION_XML);
            }
            EXPECT_TRUE(controller->outbreakMode());
        }
    };

}

TEST_F(TestOutbreakModeController, construction)
{
    EXPECT_NO_THROW(std::make_shared<OutbreakModeController>());
}

TEST_F(TestOutbreakModeController, outbreak_mode_logged)
{
    const std::string detection_xml = DETECTION_XML;
    UsingMemoryAppender recorder(*this);

    auto controller = std::make_shared<OutbreakModeController>();

    for (auto i=0; i<OUTBREAK_COUNT+1; i++)
    {
        controller->recordEventAndDetermineIfItShouldBeDropped("CORE", detection_xml);
    }

    EXPECT_TRUE(waitForLog("Entering outbreak mode"));
    EXPECT_TRUE(controller->outbreakMode());
}


TEST_F(TestOutbreakModeController, do_not_enter_outbreak_mode_for_cleanups)
{
    const std::string event_xml = R"(<?xml version="1.0" encoding="utf-8"?>
<event type="sophos.core.clean" ts="1970-01-01T00:02:03.000Z">
  <alert id="fedcba98-7654-3210-fedc-ba9876543210" succeeded="1" origin="1">
    <items totalItems="1">
      <item type="file" result="0">
        <descriptor>/path/to/eicar.txt</descriptor>
      </item>
    </items>
  </alert>
</event>)";

    UsingMemoryAppender recorder(*this);

    auto controller = std::make_shared<OutbreakModeController>();

    for (auto i=0; i<OUTBREAK_COUNT+1; i++)
    {
        controller->recordEventAndDetermineIfItShouldBeDropped("CORE", event_xml);
    }

    EXPECT_FALSE(appenderContains("Entering outbreak mode"));
    EXPECT_FALSE(controller->outbreakMode());
}

TEST_F(TestOutbreakModeController, insufficient_alerts_do_not_enter_outbreak_mode)
{
    const std::string event_xml = DETECTION_XML;
    UsingMemoryAppender recorder(*this);
    auto controller = std::make_shared<OutbreakModeController>();

    for (auto i=0; i<OUTBREAK_COUNT-1; i++)
    {
        controller->recordEventAndDetermineIfItShouldBeDropped("CORE", event_xml);
    }

    EXPECT_FALSE(appenderContains("Entering outbreak mode"));
    EXPECT_FALSE(controller->outbreakMode());
}


TEST_F(TestOutbreakModeController, alerts_over_two_days_do_not_enter_outbreak_mode)
{
    const std::string event_xml = DETECTION_XML;
    UsingMemoryAppender recorder(*this);
    auto controller = std::make_shared<OutbreakModeController>();

    auto now = OutbreakModeController::clock_t::now();
    auto yesterday = now - std::chrono::hours{24};

    for (auto i=0; i<OUTBREAK_COUNT-1; i++)
    {
        controller->recordEventAndDetermineIfItShouldBeDropped("CORE", event_xml, yesterday);
    }

    for (auto i=0; i<OUTBREAK_COUNT-1; i++)
    {
        controller->recordEventAndDetermineIfItShouldBeDropped("CORE", event_xml, now);
    }

    EXPECT_FALSE(appenderContains("Entering outbreak mode"));
    EXPECT_FALSE(controller->outbreakMode());
}

// Verify AC:  "Outbreak persists over calendar day changes"
TEST_F(TestOutbreakModeController, we_do_not_enter_outbreak_mode_if_we_are_already_in_it)
{
    const std::string event_xml = DETECTION_XML;
    UsingMemoryAppender recorder(*this);
    auto controller = std::make_shared<OutbreakModeController>();

    auto now = OutbreakModeController::clock_t::now();
    auto yesterday = now - std::chrono::hours{24};

    // Receive 101 events yesterday to go into outbreak mode
    for (auto i=0; i<OUTBREAK_COUNT+1; i++)
    {
        controller->recordEventAndDetermineIfItShouldBeDropped("CORE", event_xml, yesterday);
    }
    ASSERT_TRUE(appenderContains("Entering outbreak mode"));
    EXPECT_TRUE(controller->outbreakMode());
    clearMemoryAppender(); // Check we don't log that we are re-entering outbreak mode

    // After we enter outbreak mode, we shouldn't enter it the next day, we should already be in it.

    for (auto i=0; i<OUTBREAK_COUNT+1; i++)
    {
        EXPECT_TRUE(controller->outbreakMode()); // We'll be in outbreak mode even after receiving only 1 alert today
        controller->recordEventAndDetermineIfItShouldBeDropped("CORE", event_xml, now);
    }
    EXPECT_FALSE(appenderContains("Entering outbreak mode")); // If we hadn't stayed in outbreak mode then we would have logged again
}

TEST_F(TestOutbreakModeController, loads_true)
{
    auto* filesystemMock = new MockFileSystem();
    std::string contents = R"({"outbreakMode":true})";
    EXPECT_CALL(*filesystemMock, readFile(expectedStatusFile_, _)).WillOnce(Return(contents));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    auto controller = std::make_shared<OutbreakModeController>();
    EXPECT_TRUE(controller->outbreakMode());
}

TEST_F(TestOutbreakModeController, loads_false)
{
    auto* filesystemMock = new MockFileSystem();
    std::string contents = R"({"outbreakMode":false})";
    EXPECT_CALL(*filesystemMock, readFile(expectedStatusFile_, _)).WillOnce(Return(contents));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    auto controller = std::make_shared<OutbreakModeController>();
    EXPECT_FALSE(controller->outbreakMode());
}

TEST_F(TestOutbreakModeController, loads_missing_key)
{
    auto* filesystemMock = new MockFileSystem();
    std::string contents = R"({})";
    EXPECT_CALL(*filesystemMock, readFile(expectedStatusFile_, _)).WillOnce(Return(contents));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    auto controller = std::make_shared<OutbreakModeController>();
    EXPECT_FALSE(controller->outbreakMode());
}

TEST_F(TestOutbreakModeController, loads_outbreak_state_empty_file)
{
    auto* filesystemMock = new MockFileSystem();
    EXPECT_CALL(*filesystemMock, readFile(expectedStatusFile_, _)).WillOnce(Return(""));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    auto controller = std::make_shared<OutbreakModeController>();
    EXPECT_FALSE(controller->outbreakMode());
}

TEST_F(TestOutbreakModeController, loads_absent_file)
{
    auto* filesystemMock = new MockFileSystem();
    EXPECT_CALL(*filesystemMock, readFile(expectedStatusFile_, _)).WillOnce(Throw(
        Common::FileSystem::IFileNotFoundException("Test text")
        ));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    auto controller = std::make_shared<OutbreakModeController>();
    EXPECT_FALSE(controller->outbreakMode());
}

TEST_F(TestOutbreakModeController, loads_directory)
{
    fs::create_directories(expectedStatusFile_);
    auto controller = std::make_shared<OutbreakModeController>();
    EXPECT_FALSE(controller->outbreakMode());
}

TEST_F(TestOutbreakModeController, loads_unreadable)
{
    if (::getuid() == 0)
    {
        // Can't run this test as root
        return;
    }
    // Current FileSystem can't differentiate between can't read and doesn't exist
    auto path = expectedStatusFile_;
    const auto contents = R"({"outbreakMode":true})";
    auto* filesystem = Common::FileSystem::fileSystem();
    filesystem->writeFileAtomically(expectedStatusFile_, contents, testDir_, 0001);
    ::chmod(expectedStatusFile_.c_str(), 0);
    auto controller = std::make_shared<OutbreakModeController>();
    EXPECT_FALSE(controller->outbreakMode()); // Can't read the file
}

TEST_F(TestOutbreakModeController, loads_not_json)
{
    auto* filesystemMock = new MockFileSystem();
    EXPECT_CALL(*filesystemMock, readFile(expectedStatusFile_, _)).WillOnce(Return("abcdef"));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    auto controller = std::make_shared<OutbreakModeController>();
    EXPECT_FALSE(controller->outbreakMode());
}

TEST_F(TestOutbreakModeController, loads_not_boolean)
{
    auto* filesystemMock = new MockFileSystem();
    std::string contents = R"({"outbreakMode":123})";
    EXPECT_CALL(*filesystemMock, readFile(expectedStatusFile_, _)).WillOnce(Return(contents));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    auto controller = std::make_shared<OutbreakModeController>();
    EXPECT_FALSE(controller->outbreakMode());
}

TEST_F(TestOutbreakModeController, saves_status_file_on_outbreak)
{
    auto* filesystemMock = new MockFileSystem();
    EXPECT_CALL(*filesystemMock, readFile(_,_)).WillOnce(Return(""));

    std::string expected_contents = R"({"outbreakMode":true})";

    EXPECT_CALL(*filesystemMock, writeFileAtomically(expectedStatusFile_, expected_contents, _, _)).WillOnce(Return());

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    auto controller = std::make_shared<OutbreakModeController>();
    EXPECT_FALSE(controller->outbreakMode());
    enterOutbreakMode(controller);
}

TEST_F(TestOutbreakModeController, outbreak_status_survives_restart)
{
    auto controller = std::make_shared<OutbreakModeController>();
    EXPECT_FALSE(controller->outbreakMode());
    enterOutbreakMode(controller);

    // Replace the controller
    controller = std::make_shared<OutbreakModeController>();
    EXPECT_TRUE(controller->outbreakMode());
}
