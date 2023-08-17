// Copyright 2023 Sophos Limited. All rights reserved.

// Class under test
#include "ManagementAgent/EventReceiverImpl/Event.h"
// Product
#include "Common/ApplicationConfigurationImpl/ApplicationPathManager.h"
#include "Common/FileSystem/IFileSystem.h"
// Test helpers
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/LogInitializedTests.h"
#include "tests/Common/Helpers/MockFileSystem.h"
// 3rd parties
#include <gtest/gtest.h>

namespace
{
    StrictMock<MockFileSystem>* createMockFileSystem()
    {
        auto filesystemMock = new StrictMock<MockFileSystem>();
        return filesystemMock;
    }

    class TestEvent : public LogInitializedTests
    {
    };
}

TEST_F(TestEvent, Construction)
{
    ManagementAgent::EventReceiverImpl::Event event{"a", "b"};
    EXPECT_EQ(event.appId_, "a");
    EXPECT_EQ(event.eventXml_, "b");
}

TEST_F(TestEvent, Send)
{
    auto filesystemMock = createMockFileSystem();

    std::string tempDir = Common::ApplicationConfiguration::applicationPathManager().getManagementAgentTempPath();
    EXPECT_CALL(
        *filesystemMock,
        writeFileAtomically(
            MatchesRegex("/opt/sophos-spl/base/mcs/event/APPID_event-.*\\.xml"), "XML", tempDir, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP))
        .WillOnce(Return());

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    ManagementAgent::EventReceiverImpl::Event event{"APPID", "XML"};
    event.send();
}

TEST_F(TestEvent, Detection)
{
    const auto XML = R"sophos(<?xml version="1.0" encoding="utf-8"?>
<event type="sophos.core.detection" ts="@@TS@@">
  <user userId="@@USER_ID@@"/>
  <alert id="@@ID@@" name="@@NAME@@" threatType="@@THREAT_TYPE@@" origin="@@ORIGIN@@" remote="@@REMOTE@@">
    <sha256>@@SHA256@@</sha256>
    <path>@@PATH@@</path>
  </alert>
</event>)sophos";
    ManagementAgent::EventReceiverImpl::Event event{"CORE", XML};
    EXPECT_TRUE(event.isCountableEvent());
    EXPECT_TRUE(event.isBlockableEvent());
}

TEST_F(TestEvent, Clean)
{
    const auto XML = R"sophos(<?xml version="1.0" encoding="utf-8"?>
<event type="sophos.core.clean" ts="@@TS@@">
  <alert id="@@CORRELATION_ID@@" succeeded="@@SUCCESS_OVERALL@@" origin="@@ORIGIN@@">
    <items totalItems="@@TOTAL_ITEMS@@">
      <item type="file" result="@@SUCCESS_DETAILED@@">
        <descriptor>@@PATH@@</descriptor>
      </item>
    </items>
  </alert>
</event>)sophos";
    ManagementAgent::EventReceiverImpl::Event event{"CORE", XML};
    EXPECT_FALSE(event.isCountableEvent());
    EXPECT_TRUE(event.isBlockableEvent());
}


TEST_F(TestEvent, Restore)
{
    const auto XML = R"sophos(<?xml version="1.0" encoding="utf-8"?>
<event type="sophos.core.restore" ts="@@TS@@">
  <alert id="@@ID@@" succeeded="@@SUCCEEDED@@">
    <items totalItems="1">
      <item type="file">
        <descriptor>@@PATH@@</descriptor>
      </item>
    </items>
  </alert>
</event>)sophos";
    ManagementAgent::EventReceiverImpl::Event event{"CORE", XML};
    EXPECT_FALSE(event.isCountableEvent());
    EXPECT_FALSE(event.isBlockableEvent());
}