// Copyright 2023 Sophos Limited. All rights reserved.

// Class under test
#include "ManagementAgent/EventReceiverImpl/Event.h"
// Product
#include "FileSystem/IFileSystem.h"
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

    EXPECT_CALL(
        *filesystemMock,
        writeFileAtomically(
            MatchesRegex("/opt/sophos-spl/base/mcs/event/APPID_event-.*\\.xml"), "XML", "/opt/sophos-spl/tmp", S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP))
        .WillOnce(Return());

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    ManagementAgent::EventReceiverImpl::Event event{"APPID", "XML"};
    event.send();
}