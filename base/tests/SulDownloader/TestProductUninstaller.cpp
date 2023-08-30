// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "MockSdds3Repository.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/Process/IProcessException.h"
#include "Common/ProcessImpl/ProcessImpl.h"
#include "SulDownloader/ProductUninstaller.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/LogInitializedTests.h"
#include "tests/Common/Helpers/MockFileSystem.h"
#include "tests/Common/Helpers/MockProcess.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using Common::DownloadReport::RepositoryStatus;

class ProductUninstallerTest : public LogInitializedTests
{
    void SetUp() override
    {
        m_fileSystemMock = new StrictMock<MockFileSystem>();
        m_replacer.replace(std::unique_ptr<Common::FileSystem::IFileSystem>(m_fileSystemMock));

        // If SOPHOS_INSTALL isn't /opt/sophos-spl this path needs to match:
        m_defaultDistributionPath = Common::FileSystem::join(
            Common::ApplicationConfiguration::applicationPathManager().getLocalSdds3DistributionRepository(),
            "product"
        ); // "/opt/sophos-spl/base/update/cache/primary/product" by default
    }

    void TearDown() override
    {
        Common::ProcessImpl::ProcessFactory::instance().restoreCreator();
    }

public:
    ProductUninstallerTest() : m_fileSystemMock(nullptr) {}

    std::vector<suldownloaderdata::DownloadedProduct> createDefaultDownloadProductList(int numberProducts)
    {
        std::vector<suldownloaderdata::DownloadedProduct> productList;

        for (int i = 1; i <= numberProducts; i++)
        {
            std::stringstream productLine;
            productLine << "product" << i;
            suldownloaderdata::ProductMetadata metadata;
            metadata.setLine(productLine.str());
            suldownloaderdata::DownloadedProduct product(metadata);
            productList.push_back(product);
        }

        return productList;
    }

    std::vector<std::string> createDefaultFileList(int numberProducts)
    {
        std::vector<std::string> fileList;

        for (int i = 1; i <= numberProducts; i++)
        {
            std::stringstream productLine;
            productLine << "product" << i << ".sh";
            fileList.push_back(productLine.str());
        }

        return fileList;
    }

    MockFileSystem* m_fileSystemMock; // BORROWED
    ::testing::StrictMock<MockSdds3Repository> m_mockSDDS3WarehouseRepository;
    std::string m_defaultDistributionPath;
    Tests::ScopedReplaceFileSystem m_replacer; 
    
};

TEST_F(ProductUninstallerTest, defaultConstructorDoesNotThrow)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
    EXPECT_NO_THROW(ProductUninstaller uninstallManager);
#pragma GCC diagnostic pop
}

TEST_F(
    ProductUninstallerTest,
    removeProductsNotDownloadedDoesNotAttemptToUninstallProductWhenNoProductsInstalled)
{
    std::vector<std::string> emptyList;
    std::vector<suldownloaderdata::DownloadedProduct> emptyProductList;
    EXPECT_CALL(*m_fileSystemMock, isDirectory(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_fileSystemMock, listFiles(_)).WillOnce(Return(emptyList));
    ProductUninstaller uninstallManager;

    EXPECT_EQ(uninstallManager.removeProductsNotDownloaded(emptyProductList, m_mockSDDS3WarehouseRepository).size(), 0);
}

TEST_F(
    ProductUninstallerTest,
    removeProductsNotDownloadedDoesNotAttemptToUninstallProductWhenInstalledProductDirectoryDoesNotExist)
{
    std::vector<std::string> emptyList;
    std::vector<suldownloaderdata::DownloadedProduct> emptyProductList;
    EXPECT_CALL(*m_fileSystemMock, isDirectory(_)).WillOnce(Return(false));
    EXPECT_CALL(*m_fileSystemMock, listFiles(_)).Times(0);
    ProductUninstaller uninstallManager;

    EXPECT_EQ(uninstallManager.removeProductsNotDownloaded(emptyProductList, m_mockSDDS3WarehouseRepository).size(), 0);
}

TEST_F(
    ProductUninstallerTest,
    removeProductsNotDownloadedDoesNotAttemptToUninstallProductWhenOnlyInstalledProductsInList)
{
    std::vector<std::string> fileList = createDefaultFileList(3);
    std::vector<suldownloaderdata::DownloadedProduct> productList = createDefaultDownloadProductList(3);
    EXPECT_CALL(*m_fileSystemMock, isDirectory(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_fileSystemMock, listFiles(_)).WillOnce(Return(fileList));
    ProductUninstaller uninstallManager;

    EXPECT_EQ(uninstallManager.removeProductsNotDownloaded(productList, m_mockSDDS3WarehouseRepository).size(), 0);
}

TEST_F(
    ProductUninstallerTest,
    removeProductsNotDownloadedDoesNotAttemptToUninstallProductWhenMoreProductsAreInDownloadListThanInstalled)
{
    std::vector<std::string> fileList = createDefaultFileList(1);
    std::vector<suldownloaderdata::DownloadedProduct> productList = createDefaultDownloadProductList(3);
    EXPECT_CALL(*m_fileSystemMock, isDirectory(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_fileSystemMock, listFiles(_)).WillOnce(Return(fileList));
    ProductUninstaller uninstallManager;

    EXPECT_EQ(uninstallManager.removeProductsNotDownloaded(productList, m_mockSDDS3WarehouseRepository).size(), 0);
}

TEST_F(
    ProductUninstallerTest,
    removeProductsNotDownloaded_ExpectToUninstallProductWhenProductNotInDownloadListButIsInstalled)
{
    std::vector<std::string> fileList = createDefaultFileList(3);
    std::vector<suldownloaderdata::DownloadedProduct> productList = createDefaultDownloadProductList(2);
    EXPECT_CALL(*m_fileSystemMock, isDirectory(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/sdds3primary/product")).WillOnce(Return(true));
    EXPECT_CALL(*m_fileSystemMock, listFiles(_)).WillOnce(Return(fileList));
    EXPECT_CALL(*m_fileSystemMock, removeFileOrDirectory(_)).Times(1);
    ProductUninstaller uninstallManager;

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&fileList]() {
        auto* mockProcess = new StrictMock<MockProcess>();
        EXPECT_CALL(*mockProcess, exec(fileList[2], _, _)).Times(1);
        EXPECT_CALL(*mockProcess, output()).WillOnce(Return("uninstalling product"));
        EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
        return std::unique_ptr<Common::Process::IProcess>(mockProcess);
    });
    EXPECT_CALL(m_mockSDDS3WarehouseRepository, getProductDistributionPath(_)).WillOnce(Return(m_defaultDistributionPath));
    std::vector<suldownloaderdata::DownloadedProduct> actualProductList;
    EXPECT_NO_THROW( 
        actualProductList = uninstallManager.removeProductsNotDownloaded(
                        productList, m_mockSDDS3WarehouseRepository));
    ASSERT_EQ(actualProductList.size(), 1);
    EXPECT_TRUE(actualProductList[0].getProductIsBeingUninstalled());

    EXPECT_EQ(actualProductList[0].getLine(), fileList[2].substr(0, fileList[2].find(".sh")));
}

TEST_F(
    ProductUninstallerTest,
    removeProductsNotDownloaded_DoNotTryToRemoveDistributeDirWhenItDoesntExist)
{
    std::vector<std::string> fileList = createDefaultFileList(3);
    std::vector<suldownloaderdata::DownloadedProduct> productList = createDefaultDownloadProductList(2);
    EXPECT_CALL(*m_fileSystemMock, isDirectory(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/sdds3primary/product")).WillOnce(Return(false));
    EXPECT_CALL(*m_fileSystemMock, listFiles(_)).WillOnce(Return(fileList));

    ProductUninstaller uninstallManager;

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&fileList]() {
                                                                       auto* mockProcess = new StrictMock<MockProcess>();
                                                                       EXPECT_CALL(*mockProcess, exec(fileList[2], _, _)).Times(1);
                                                                       EXPECT_CALL(*mockProcess, output()).WillOnce(Return("uninstalling product"));
                                                                       EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
                                                                       return std::unique_ptr<Common::Process::IProcess>(mockProcess);
                                                                   });
    EXPECT_CALL(m_mockSDDS3WarehouseRepository, getProductDistributionPath(_)).WillOnce(Return(m_defaultDistributionPath));
    std::vector<suldownloaderdata::DownloadedProduct> actualProductList;
    EXPECT_NO_THROW( 
        actualProductList = uninstallManager.removeProductsNotDownloaded(
            productList, m_mockSDDS3WarehouseRepository));
    ASSERT_EQ(actualProductList.size(), 1);
    EXPECT_TRUE(actualProductList[0].getProductIsBeingUninstalled());

    EXPECT_EQ(actualProductList[0].getLine(), fileList[2].substr(0, fileList[2].find(".sh")));
}

TEST_F(ProductUninstallerTest, removeProductsNotDownloaded_FailureToUninstallProductThrowsAndIsHandled)
{
    std::vector<std::string> fileList = createDefaultFileList(3);
    std::vector<suldownloaderdata::DownloadedProduct> productList = createDefaultDownloadProductList(2);
    EXPECT_CALL(*m_fileSystemMock, isDirectory(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_fileSystemMock, listFiles(_)).WillOnce(Return(fileList));
    ProductUninstaller uninstallManager;

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&fileList]() {
        auto mockProcess = new StrictMock<MockProcess>();
        EXPECT_CALL(*mockProcess, exec(fileList[2], _, _))
            .WillOnce(Throw(Common::Process::IProcessException("ProcessThrow")));
        return std::unique_ptr<Common::Process::IProcess>(mockProcess);
    });
    EXPECT_CALL(m_mockSDDS3WarehouseRepository, getProductDistributionPath(_)).WillOnce(Return(m_defaultDistributionPath));
    std::vector<suldownloaderdata::DownloadedProduct> actualProductList;
    EXPECT_NO_THROW(actualProductList = uninstallManager.removeProductsNotDownloaded(
                        productList, m_mockSDDS3WarehouseRepository));
    EXPECT_EQ(actualProductList.size(), 1);
    EXPECT_TRUE(actualProductList[0].getProductIsBeingUninstalled());
    EXPECT_EQ(actualProductList[0].getLine(), fileList[2].substr(0, fileList[2].find(".sh")));
    EXPECT_TRUE(actualProductList[0].getError().Description.find("ProcessThrow") != std::string::npos);
    EXPECT_EQ(actualProductList[0].getError().status, RepositoryStatus::UNINSTALLFAILED);
    EXPECT_EQ(actualProductList[0].getError().LibError, "");
}

TEST_F(ProductUninstallerTest, removeProductsNotDownloaded_FailureToUninstallProductReturnsNonZeroErrorCode)
{
    std::vector<std::string> fileList = createDefaultFileList(3);
    std::vector<suldownloaderdata::DownloadedProduct> productList = createDefaultDownloadProductList(2);
    EXPECT_CALL(*m_fileSystemMock, isDirectory(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_fileSystemMock, listFiles(_)).WillOnce(Return(fileList));
    ProductUninstaller uninstallManager;

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&fileList]() {
        auto mockProcess = new StrictMock<MockProcess>();
        EXPECT_CALL(*mockProcess, exec(fileList[2], _, _)).Times(1);
        EXPECT_CALL(*mockProcess, output()).WillOnce(Return("uninstalling product failed"));
        EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(1));
        return std::unique_ptr<Common::Process::IProcess>(mockProcess);
    });
    EXPECT_CALL(m_mockSDDS3WarehouseRepository, getProductDistributionPath(_)).WillOnce(Return(m_defaultDistributionPath));
    std::vector<suldownloaderdata::DownloadedProduct> actualProductList;
    EXPECT_NO_THROW(actualProductList = uninstallManager.removeProductsNotDownloaded(
                        productList, m_mockSDDS3WarehouseRepository));
    EXPECT_EQ(actualProductList.size(), 1);
    EXPECT_TRUE(actualProductList[0].getProductIsBeingUninstalled());
    EXPECT_EQ(actualProductList[0].getLine(), fileList[2].substr(0, fileList[2].find(".sh")));
    EXPECT_TRUE(
        actualProductList[0].getError().Description.find("Process did not complete successfully") != std::string::npos);
    EXPECT_EQ(actualProductList[0].getError().status, RepositoryStatus::UNINSTALLFAILED);
    EXPECT_EQ(actualProductList[0].getError().LibError, "");
}

TEST_F(
    ProductUninstallerTest,
    shouldRefuseToRemoveSlashDirectory)
{
    std::vector<std::string> fileList = createDefaultFileList(3);
    std::vector<suldownloaderdata::DownloadedProduct> productList = createDefaultDownloadProductList(2);
    EXPECT_CALL(*m_fileSystemMock, isDirectory(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_fileSystemMock, listFiles(_)).WillOnce(Return(fileList));
    EXPECT_CALL(*m_fileSystemMock, removeFileOrDirectory(_))
        .Times(0); // being explicitly that it should not remove file
    ProductUninstaller uninstallManager;

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&fileList]() {
        auto mockProcess = new StrictMock<MockProcess>();
        EXPECT_CALL(*mockProcess, exec(fileList[2], _, _)).Times(1);
        EXPECT_CALL(*mockProcess, output()).WillOnce(Return("uninstalling product"));
        EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
        return std::unique_ptr<Common::Process::IProcess>(mockProcess);
    });
    // simulate warehouse repository returning / root directory to be remove (this would be a mistake, but we want
    // ProductUninstaller to not remove the / directory)
    EXPECT_CALL(m_mockSDDS3WarehouseRepository, getProductDistributionPath(_)).WillOnce(Return("/"));

    EXPECT_THROW(
        uninstallManager.removeProductsNotDownloaded(productList, m_mockSDDS3WarehouseRepository),
        std::logic_error);
}
