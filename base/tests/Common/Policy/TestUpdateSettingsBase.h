// Copyright 2023 Sophos All rights reserved.

#pragma once

#include "Common/Policy/UpdateSettings.h"

#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/MemoryAppender.h"
#include "tests/Common/Helpers/MockFileSystem.h"

namespace
{
    class TestUpdateSettingsBase : public MemoryAppenderUsingTests
    {
    public:
        TestUpdateSettingsBase() : MemoryAppenderUsingTests("Policy") {}

        static Common::Policy::UpdateSettings getValidUpdateSettings()
        {
            Common::Policy::UpdateSettings validSettings;
            validSettings.setSophosLocationURLs({"http://really_sophos.info"});
            validSettings.setCredentials({"username", "password"});
            validSettings.setPrimarySubscription({"RIGID", "Base", "tag", "fixed"});
            validSettings.setFeatures({"CORE"});
            return validSettings;
        }

        MockFileSystem& setupFileSystemAndGetMock()
        {
            using ::testing::Ne;
            Common::ApplicationConfiguration::applicationConfiguration().setData(
                Common::ApplicationConfiguration::SOPHOS_INSTALL, "/installroot");

            auto filesystemMock = std::make_unique<NiceMock<MockFileSystem>>();
            ON_CALL(*filesystemMock, isDirectory(Common::ApplicationConfiguration::applicationPathManager().sophosInstall())).WillByDefault(Return(true));
            ON_CALL(*filesystemMock, isDirectory(Common::ApplicationConfiguration::applicationPathManager().getLocalWarehouseStoreDir())).WillByDefault(Return(true));

            std::string empty;
            ON_CALL(*filesystemMock, exists(empty)).WillByDefault(Return(false));
            ON_CALL(*filesystemMock, exists(Ne(empty))).WillByDefault(Return(true));

            auto* borrowedPtr = filesystemMock.get();
            m_replacer.replace(std::move(filesystemMock));
            return *borrowedPtr;
        }

        Tests::ScopedReplaceFileSystem m_replacer;
    };
}
