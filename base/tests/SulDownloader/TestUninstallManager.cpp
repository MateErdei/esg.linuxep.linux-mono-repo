/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <SulDownloader/UninstallManager.h>
#include <tests/Common/FileSystemImpl/MockFileSystem.h>
#include <modules/Common/FileSystemImpl/FileSystemImpl.h>
#include <modules/Common/ProcessImpl/ProcessImpl.h>

using namespace SulDownloader;

class UninstallManagerTest : public ::testing::Test
{
    void SetUp() override
    {
        m_fileSystemMock = new StrictMock<MockFileSystem>();
        Common::FileSystem::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(m_fileSystemMock));

    }

    void TearDown() override
    {
        Common::FileSystem::restoreFileSystem();
        Common::ProcessImpl::ProcessFactory::instance().restoreCreator();
    }

public:
    MockFileSystem* m_fileSystemMock;

};

TEST_F(UninstallManagerTest, defaultConstructorDoesNotThrow)
{
    EXPECT_NO_THROW(UninstallManager uninstallManager;);
}

TEST_F(UninstallManagerTest, PerformCleanUpDoesNotAttemptToUninstallProductWhenNoProductsInstalled)
{
    std::vector<std::string> emptyList;
    std::vector<DownloadedProduct> emptyProductList;
    EXPECT_CALL(*m_fileSystemMock, listFiles(_)).WillOnce(Return(emptyList));
    UninstallManager uninstallManager;

    EXPECT_EQ(uninstallManager.performCleanUp(emptyProductList).size(), 0);
}

//Common::ProcessImpl::ProcessFactory::instance().replaceCreator([]() {
//auto mockProcess = new MockProcess();
//EXPECT_CALL(*mockProcess, exec(HasSubstr("productRemove1"), _, _)).Times(1);
//EXPECT_CALL(*mockProcess, output()).WillOnce(Return("uninstalling product"));
//EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
//return std::unique_ptr<Common::Process::IProcess>(mockProcess);
//}
//);