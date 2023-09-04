// Copyright 2023 Sophos Limited. All rights reserved.

#define TEST_PUBLIC public

#include "Common/FileSystem/IFileNotFoundException.h"
#include "Common/FileSystem/IFileTooLargeException.h"
#include "ManagementAgent/EventReceiverImpl/OutbreakModeController.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/MockFilePermissions.h"
#include "tests/Common/Helpers/MockFileSystem.h"
#include "tests/Common/Helpers/TestSpecificDirectory.h"
#include "tests/Common/UtilityImpl/MockFormattedTime.h"

using namespace ManagementAgent::EventReceiverImpl;
namespace fs = sophos_filesystem;

namespace
{
    const int OUTBREAK_COUNT = 100;

    class TestableOutbreakModeController : public OutbreakModeController
    {
    public:
        using OutbreakModeController::OutbreakModeController;
        int getDetectionCount()
        {
            return detectionCount_;
        }
    };

    const std::string DETECTION_XML = R"sophos(<?xml version="1.0" encoding="utf-8"?>
<event type="sophos.core.detection" ts="1970-01-01T00:02:03.000Z">
  <user userId="username"/>
  <alert id="fedcba98-7654-3210-fedc-ba9876543210" name="EICAR-AV-Test" threatType="1" origin="1" remote="true">
    <sha256>131f95c51cc819465fa1797f6ccacf9d494aaaff46fa3eac73ae63ffbdfd8267</sha256>
    <path>/path/to/eicar.txt</path>
  </alert>
</event>)sophos";

    const std::string CLEANUP_XML = R"(<?xml version="1.0" encoding="utf-8"?>
<event type="sophos.core.clean" ts="1970-01-01T00:02:03.000Z">
  <alert id="fedcba98-7654-3210-fedc-ba9876543210" succeeded="1" origin="1">
    <items totalItems="1">
      <item type="file" result="0">
        <descriptor>/path/to/eicar.txt</descriptor>
      </item>
    </items>
  </alert>
</event>)";

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
            varDir_ = testDir_ / "var" / "sophosspl";
            fs::create_directories(varDir_);
            eventDir_ = testDir_ / "base" / "mcs" / "event";
            fs::create_directories(eventDir_);
            expectedStatusFile_ = testDir_ / "var" / "sophosspl" / "outbreak_status.json";
        }

        void TearDown() override
        {
            test_common::removeTestSpecificDirectory(testDir_);
        }

        fs::path testDir_;
        fs::path varDir_;
        fs::path eventDir_;
        Path expectedStatusFile_;

        static bool processEventThrowAwayArgs(const OutbreakModeControllerPtr& controller,
                                              const std::string& appID,
                                              const std::string& xml)
        {
            return controller->processEvent({appID, xml});
        }

        static bool processEventThrowAwayArgs(const OutbreakModeControllerPtr& controller,
                                              const std::string& appID,
                                              const std::string& xml,
                                              OutbreakModeController::time_point_t now)
        {
            return controller->processEvent({appID, xml}, now);
        }

        static void repeatProcessEvent(const OutbreakModeControllerPtr& controller,
                                       const std::string& xml, int count, const std::string& appID="CORE")
        {
            for (auto i=0; i<count; i++)
            {
                std::ignore = processEventThrowAwayArgs(controller, appID, xml);
            }
        }

        static void enterOutbreakMode(const OutbreakModeControllerPtr& controller)
        {
            repeatProcessEvent(controller, DETECTION_XML, OUTBREAK_COUNT+1);
            EXPECT_TRUE(controller->outbreakMode());
        }

        void checkOutbreakEventSent();
        std::unique_ptr<MockFormattedTime> m_formattedTime;
    };

    void TestOutbreakModeController::checkOutbreakEventSent()
    {
        auto* filesystem = Common::FileSystem::fileSystem();
        // List eventDir_
        auto files = filesystem->listFiles(eventDir_);
        EXPECT_EQ(files.size(), 1);
        if (files.size() == 1)
        {
            auto path = files[0];
            auto name = Common::FileSystem::basename(path);
            auto contents = filesystem->readFile(path);
            EXPECT_THAT(contents, HasSubstr("sophos.core.outbreak"));
            EXPECT_THAT(name, StartsWith("CORE_event-"));
            EXPECT_THAT(name, EndsWith(".xml"));
        }
    }
}

TEST_F(TestOutbreakModeController, construction)
{
    EXPECT_NO_THROW(std::make_shared<OutbreakModeController>());
}

TEST_F(TestOutbreakModeController, handle_invalid_event_xml)
{
    /*
     * Still counted, since we are just doing prefix string matching for events.
     */
    std::string xml = R"sophos(<?xml version="1.0" encoding="utf-8"?>
<event type="sophos.core.detection" ts="1970-01-01T00:02:03.000Z">
  <user userId="username"/>
  <alert id="fedcba98)sophos";
    auto controller = std::make_shared<OutbreakModeController>();
    EXPECT_NO_THROW(processEventThrowAwayArgs(controller, "CORE", xml));
}

TEST_F(TestOutbreakModeController, entering_outbreak_mode)
{
    const std::string detection_xml = DETECTION_XML;
    UsingMemoryAppender recorder(*this);

    auto controller = std::make_shared<OutbreakModeController>();

    for (auto i=0; i<OUTBREAK_COUNT-1; i++)
    {
        processEventThrowAwayArgs(controller, "CORE", detection_xml);
    }
    std::string appId{"CORE"};
    std::string xml{detection_xml};
    auto result = controller->processEvent({appId, xml});

    EXPECT_FALSE(result); // keep the event
    EXPECT_EQ(xml, detection_xml); // Still report the event

    // Check we have an event reported for outbreak mode
    checkOutbreakEventSent();

    EXPECT_TRUE(waitForLog("Entering outbreak mode"));
    EXPECT_TRUE(waitForLog("Generating Outbreak notification with UUID=5df69683-a5a2-5d96-897d-06f9c4c8c7bf"));
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
        processEventThrowAwayArgs(controller, "CORE", event_xml);
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
        processEventThrowAwayArgs(controller, "CORE", event_xml);
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
        processEventThrowAwayArgs(controller, "CORE", event_xml, yesterday);
    }

    for (auto i=0; i<OUTBREAK_COUNT-1; i++)
    {
        processEventThrowAwayArgs(controller, "CORE", event_xml, now);
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
        processEventThrowAwayArgs(controller, "CORE", event_xml, yesterday);
    }
    ASSERT_TRUE(appenderContains("Entering outbreak mode"));
    EXPECT_TRUE(controller->outbreakMode());
    clearMemoryAppender(); // Check we don't log that we are re-entering outbreak mode

    // After we enter outbreak mode, we shouldn't enter it the next day, we should already be in it.

    for (auto i=0; i<OUTBREAK_COUNT+1; i++)
    {
        EXPECT_TRUE(controller->outbreakMode()); // We'll be in outbreak mode even after receiving only 1 alert today
        processEventThrowAwayArgs(controller, "CORE", event_xml, now);
    }
    EXPECT_FALSE(appenderContains("Entering outbreak mode")); // If we hadn't stayed in outbreak mode then we would have logged again
}

TEST_F(TestOutbreakModeController, loads_true)
{
    auto mockFileSystem =  std::make_unique<StrictMock<MockFileSystem>>();
    std::string contents = R"({"outbreak-mode":true})";
    EXPECT_CALL(*mockFileSystem, readFile(expectedStatusFile_, _)).WillOnce(Return(contents));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockFileSystem));

    auto controller = std::make_shared<OutbreakModeController>();
    EXPECT_TRUE(controller->outbreakMode());
}

TEST_F(TestOutbreakModeController, loads_false)
{
    UsingMemoryAppender recorder(*this);
    auto mockFileSystem =  std::make_unique<StrictMock<MockFileSystem>>();
    std::string contents = R"({"outbreak-mode":false})";
    EXPECT_CALL(*mockFileSystem, readFile(expectedStatusFile_, _)).WillOnce(Return(contents));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockFileSystem));

    auto controller = std::make_shared<OutbreakModeController>();
    EXPECT_FALSE(controller->outbreakMode());
    EXPECT_FALSE(appenderContains("ERROR"));
}

TEST_F(TestOutbreakModeController, loads_missing_key)
{
    auto mockFileSystem =  std::make_unique<StrictMock<MockFileSystem>>();
    std::string contents = R"({})";
    EXPECT_CALL(*mockFileSystem, readFile(expectedStatusFile_, _)).WillOnce(Return(contents));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockFileSystem));

    auto controller = std::make_shared<OutbreakModeController>();
    EXPECT_FALSE(controller->outbreakMode());
}

TEST_F(TestOutbreakModeController, loads_outbreak_state_empty_file)
{
    auto mockFileSystem =  std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, readFile(expectedStatusFile_, _)).WillOnce(Return(""));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockFileSystem));

    auto controller = std::make_shared<OutbreakModeController>();
    EXPECT_FALSE(controller->outbreakMode());
}

TEST_F(TestOutbreakModeController, loads_absent_file)
{
    auto mockFileSystem =  std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, readFile(expectedStatusFile_, _)).WillOnce(Throw(
        Common::FileSystem::IFileNotFoundException("Test text")
        ));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockFileSystem));

    auto controller = std::make_shared<OutbreakModeController>();
    EXPECT_FALSE(controller->outbreakMode());
}

TEST_F(TestOutbreakModeController, loads_large_file)
{
    auto mockFileSystem =  std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, readFile(expectedStatusFile_, _)).WillOnce(Throw(
        Common::FileSystem::IFileTooLargeException("Test text")
            ));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockFileSystem));

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
    const auto contents = R"({"outbreak-mode":true})";
    auto* filesystem = Common::FileSystem::fileSystem();
    filesystem->writeFileAtomically(expectedStatusFile_, contents, testDir_, 0001);
    ::chmod(expectedStatusFile_.c_str(), 0);
    auto controller = std::make_shared<OutbreakModeController>();
    EXPECT_FALSE(controller->outbreakMode()); // Can't read the file
}

TEST_F(TestOutbreakModeController, loads_too_large)
{
    UsingMemoryAppender recorder(*this);
    auto path = expectedStatusFile_;
    const int SIZE = 1025; // file size
    std::ostringstream large_string_stream;
    large_string_stream << "{\"outbreak-mode\":";
    for (int i = 0; i < SIZE / 10; ++i)
    {
        large_string_stream << "0123456789";
    }
    large_string_stream << "}";
    std::string contents = large_string_stream.str();
    auto* filesystem = Common::FileSystem::fileSystem();
    filesystem->writeFileAtomically(expectedStatusFile_, contents, testDir_, 0640);
    auto controller = std::make_shared<OutbreakModeController>();
    EXPECT_FALSE(controller->outbreakMode()); // Can't read the file
    EXPECT_TRUE(appenderContains("file too large"));
}

TEST_F(TestOutbreakModeController, timestamp_in_future)
{
    auto tmrw = OutbreakModeController::clock_t::now() + std::chrono::hours(24);
    auto timestamp = Common::UtilityImpl::TimeUtils::MessageTimeStamp(tmrw);
    auto filesystemMock = std::make_unique<MockFileSystem>();
    std::stringstream contents;
    contents << "{\"outbreak-mode\":true,\"" << timestamp << "\":\".*\",\"uuid\":\"5df69683-a5a2-5d96-897d-06f9c4c8c7bf\"}";
    EXPECT_CALL(*filesystemMock, readFile(expectedStatusFile_, _)).WillOnce(Return(contents.str()));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(std::move(filesystemMock))};

    auto controller = std::make_shared<OutbreakModeController>();
    EXPECT_TRUE(controller->outbreakMode());
}

TEST_F(TestOutbreakModeController, loads_not_json)
{
    auto mockFileSystem =  std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, readFile(expectedStatusFile_, _)).WillOnce(Return("abcdef"));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockFileSystem));

    auto controller = std::make_shared<OutbreakModeController>();
    EXPECT_FALSE(controller->outbreakMode());
}

TEST_F(TestOutbreakModeController, loads_not_boolean)
{
    auto mockFileSystem =  std::make_unique<StrictMock<MockFileSystem>>();
    std::string contents = R"({"outbreak-mode":123})";
    EXPECT_CALL(*mockFileSystem, readFile(expectedStatusFile_, _)).WillOnce(Return(contents));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockFileSystem));

    auto controller = std::make_shared<OutbreakModeController>();
    EXPECT_FALSE(controller->outbreakMode());
}

TEST_F(TestOutbreakModeController, loads_uuid_not_string)
{
    auto mockFileSystem =  std::make_unique<StrictMock<MockFileSystem>>();
    std::string contents = R"({"outbreak-mode":true, "uuid":false})";
    EXPECT_CALL(*mockFileSystem, readFile(expectedStatusFile_, _)).WillOnce(Return(contents));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockFileSystem));

    OutbreakModeControllerPtr controller;
    ASSERT_NO_THROW(controller = std::make_shared<OutbreakModeController>());
    EXPECT_TRUE(controller->outbreakMode()); // outbreak mode loaded first ???
}

TEST_F(TestOutbreakModeController, saves_status_file_on_outbreak)
{
    auto mockFileSystem =  std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, readFile(_,_)).WillOnce(Return(""));

    auto now = OutbreakModeController::clock_t::now();
    auto timestamp = Common::UtilityImpl::TimeUtils::MessageTimeStamp(now);

    // because mockable time not being used
    std::string expectedContentRegex = R"(.\"outbreak-mode\":true,\"timestamp\":\".*\",\"uuid\":\"5df69683-a5a2-5d96-897d-06f9c4c8c7bf\".)";

    EXPECT_CALL(*mockFileSystem, writeFileAtomically(expectedStatusFile_, MatchesRegex(expectedContentRegex), _, _)).WillOnce(Return());
    EXPECT_CALL(*mockFileSystem, writeFileAtomically(
                                     HasSubstr("base/mcs/event/CORE_event-"),
                                     HasSubstr("sophos.core.outbreak"), _, _)).WillOnce(Return());

    auto mockFilePermissions = std::make_unique<MockFilePermissions>();
#ifndef SPL_BAZEL
    EXPECT_CALL(*mockFilePermissions, chown(expectedStatusFile_,"root","sophos-spl-group")).WillOnce(Return());
#endif

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockFileSystem));
    Tests::ScopedReplaceFilePermissions scopedReplaceFilePermissions(std::move(mockFilePermissions));

    auto controller = std::make_shared<OutbreakModeController>();
    EXPECT_FALSE(controller->outbreakMode());
    enterOutbreakMode(controller);
}

TEST_F(TestOutbreakModeController, outbreak_status_survives_restart)
{
    auto controller = std::make_shared<OutbreakModeController>();
    EXPECT_FALSE(controller->outbreakMode());
    enterOutbreakMode(controller);
    ASSERT_TRUE(controller->outbreakMode());
    controller.reset();

    // Replace the controller
    controller = std::make_shared<OutbreakModeController>();
    EXPECT_TRUE(controller->outbreakMode());
}

TEST_F(TestOutbreakModeController, write_fails_no_directory)
{
    fs::remove(varDir_); // Remove the directory we are supposed to write to

    UsingMemoryAppender recorder(*this);
    auto controller = std::make_shared<OutbreakModeController>();
    EXPECT_FALSE(controller->outbreakMode());
    EXPECT_NO_THROW(enterOutbreakMode(controller));
    EXPECT_TRUE(controller->outbreakMode());
    EXPECT_TRUE(appenderContains("No such file or directory"));
}

TEST_F(TestOutbreakModeController, write_fails_no_permission)
{
    if (::getuid() == 0)
    {
        // Can't run this test as root
        return;
    }
    UsingMemoryAppender recorder(*this);
    ::chmod(varDir_.c_str(), 0);
    auto controller = std::make_shared<OutbreakModeController>();
    EXPECT_FALSE(controller->outbreakMode());
    EXPECT_NO_THROW(enterOutbreakMode(controller));
    EXPECT_TRUE(controller->outbreakMode());
    EXPECT_TRUE(appenderContains("Permission denied"));
}

TEST_F(TestOutbreakModeController, write_fails_directory_in_place_of_file)
{
    UsingMemoryAppender recorder(*this);
    // Create a directory where we are supposed to write the status file
    fs::create_directories(expectedStatusFile_);

    auto controller = std::make_shared<OutbreakModeController>();
    EXPECT_FALSE(controller->outbreakMode());
    EXPECT_NO_THROW(enterOutbreakMode(controller));
    EXPECT_TRUE(controller->outbreakMode());
    EXPECT_TRUE(appenderContains("Is a directory"));
}

TEST_F(TestOutbreakModeController, write_fails_permission_on_file)
{
    const auto contents = R"({"outbreak-mode":123})";
    auto* filesystem = Common::FileSystem::fileSystem();
    filesystem->writeFileAtomically(expectedStatusFile_, contents, testDir_, 0001);
    ::chmod(expectedStatusFile_.c_str(), 0);

    auto controller = std::make_shared<OutbreakModeController>();
    EXPECT_FALSE(controller->outbreakMode());
    EXPECT_NO_THROW(enterOutbreakMode(controller));
    EXPECT_TRUE(controller->outbreakMode());
}

TEST_F(TestOutbreakModeController, action_leaves_outbreak_mode)
{
    auto controller = std::make_shared<OutbreakModeController>();
    EXPECT_FALSE(controller->outbreakMode());
    enterOutbreakMode(controller);
    ASSERT_TRUE(controller->outbreakMode());

    controller->processAction(R"(<?xml version="1.0"?><action type="sophos.core.threat.sav.clear"><item id="5df69683-a5a2-5d96-897d-06f9c4c8c7bf"/></action>)");
    EXPECT_FALSE(controller->outbreakMode());
}

TEST_F(TestOutbreakModeController, leaving_outbreak_mode_is_persisted)
{
    auto controller = std::make_shared<OutbreakModeController>();
    EXPECT_FALSE(controller->outbreakMode());
    enterOutbreakMode(controller);
    ASSERT_TRUE(controller->outbreakMode());
    controller->processAction(R"(<?xml version="1.0"?><action type="sophos.core.threat.sav.clear"><item id="5df69683-a5a2-5d96-897d-06f9c4c8c7bf"/></action>)");
    ASSERT_FALSE(controller->outbreakMode());

    controller.reset();

    // Replace the controller
    controller = std::make_shared<OutbreakModeController>();
    EXPECT_FALSE(controller->outbreakMode());
}

TEST_F(TestOutbreakModeController, leaving_outbreak_mode_resets_count)
{
    auto controller = std::make_shared<OutbreakModeController>();
    EXPECT_FALSE(controller->outbreakMode());
    enterOutbreakMode(controller);
    ASSERT_TRUE(controller->outbreakMode());
    controller->processAction(R"(<?xml version="1.0"?><action type="sophos.core.threat.sav.clear"><item id="5df69683-a5a2-5d96-897d-06f9c4c8c7bf"/></action>)");
    ASSERT_FALSE(controller->outbreakMode());

    bool drop = processEventThrowAwayArgs(controller, "CORE", DETECTION_XML);
    EXPECT_FALSE(drop);
    EXPECT_FALSE(controller->outbreakMode());

    int count = 1; // done one already
    while (!drop && count < OUTBREAK_COUNT * 2)
    {
        drop = processEventThrowAwayArgs(controller, "CORE", DETECTION_XML);
        count += 1;
    }
    EXPECT_EQ(count, OUTBREAK_COUNT + 1); // counts the first drop
    EXPECT_TRUE(controller->outbreakMode());
}

TEST_F(TestOutbreakModeController, ignore_irrelevant_action)
{
    auto controller = std::make_shared<OutbreakModeController>();
    EXPECT_FALSE(controller->outbreakMode());
    enterOutbreakMode(controller);
    ASSERT_TRUE(controller->outbreakMode());

    controller->processAction(R"(<?xml version="1.0"?><action type="sophos.core.do.something"><item id="5df69683-a5a2-5d96-897d-06f9c4c8c7bf"/></action>)");
    EXPECT_TRUE(controller->outbreakMode());
}

TEST_F(TestOutbreakModeController, ignore_missing_item_in_action_xml)
{
    auto controller = std::make_shared<OutbreakModeController>();
    EXPECT_FALSE(controller->outbreakMode());
    enterOutbreakMode(controller);
    ASSERT_TRUE(controller->outbreakMode());

    controller->processAction(R"(<?xml version="1.0"?><action type="sophos.core.threat.sav.clear"></action>)");
    EXPECT_TRUE(controller->outbreakMode());
}

TEST_F(TestOutbreakModeController, ignore_broken_action_xml)
{
    auto controller = std::make_shared<OutbreakModeController>();
    EXPECT_FALSE(controller->outbreakMode());
    enterOutbreakMode(controller);
    ASSERT_TRUE(controller->outbreakMode());

    controller->processAction(R"(<?xml version="1.0"?><action)");
    EXPECT_TRUE(controller->outbreakMode());
}

TEST_F(TestOutbreakModeController, ignore_wrong_id_clearing_outbreak_mode)
{
    auto mockFileSystem =  std::make_unique<StrictMock<MockFileSystem>>();
    std::string contents = R"({"outbreak-mode":true,"uuid":"5df69683-a5a2-5d96-897d-06f9c4c8c7bf"})";
    EXPECT_CALL(*mockFileSystem, readFile(expectedStatusFile_, _)).WillOnce(Return(contents));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockFileSystem));

    auto controller = std::make_shared<OutbreakModeController>();
    ASSERT_TRUE(controller->outbreakMode());

    controller->processAction(
        R"(<?xml version="1.0"?><action type="sophos.core.threat.sav.clear"><item id="5df69683-a5a2-ffff-ffff-06f9c4c8c7bf"/></action>)");
    EXPECT_TRUE(controller->outbreakMode());
}

TEST_F(TestOutbreakModeController, reset_action_leaves_outbreak_mode)
{
    auto controller = std::make_shared<TestableOutbreakModeController>();
    EXPECT_FALSE(controller->outbreakMode());
    enterOutbreakMode(controller);
    ASSERT_TRUE(controller->outbreakMode());
    bool drop = processEventThrowAwayArgs(controller, "CORE", DETECTION_XML);
    EXPECT_TRUE(drop);
    drop = processEventThrowAwayArgs(controller, "CORE", CLEANUP_XML);
    EXPECT_TRUE(drop);

    controller->processAction(R"(<?xml version="1.0"?><action type="sophos.core.threat.reset"/>)");
    EXPECT_FALSE(controller->outbreakMode());
    EXPECT_EQ(controller->getDetectionCount(), 0);
    drop = processEventThrowAwayArgs(controller, "CORE", DETECTION_XML);
    EXPECT_FALSE(drop);
    drop = processEventThrowAwayArgs(controller, "CORE", CLEANUP_XML);
    EXPECT_FALSE(drop);
}
