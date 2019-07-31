/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "MockWarehouseRepository.h"

#include <Common/FileSystemImpl/FileSystemImpl.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/Process/IProcessException.h>
#include <Common/ProcessImpl/ProcessImpl.h>
#include <SulDownloader/ProductUninstaller.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/MockFileSystem.h>
#include <tests/Common/ProcessImpl/MockProcess.h>

using namespace SulDownloader;

class ProductUninstallerTest : public ::testing::Test
{
    Common::Logging::ConsoleLoggingSetup m_consoleLogging;

    void SetUp() override
    {
        m_fileSystemMock = new StrictMock<MockFileSystem>();
        Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(m_fileSystemMock));
        m_defaultDistributionPath = "/opt/sophos-spl/base/update/cache/primary/product";
    }

    void TearDown() override
    {
        Tests::restoreFileSystem();
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
    ::testing::StrictMock<MockWarehouseRepository> m_mockWarehouseRepository;
    std::string m_defaultDistributionPath;
};

TEST_F(ProductUninstallerTest, defaultConstructorDoesNotThrow) // NOLINT
{
    EXPECT_NO_THROW(ProductUninstaller uninstallManager;); // NOLINT
}

TEST_F( // NOLINT
    ProductUninstallerTest,
    removeProductsNotDownloadedDoesNotAttemptToUninstallProductWhenNoProductsInstalled)
{
    std::vector<std::string> emptyList;
    std::vector<suldownloaderdata::DownloadedProduct> emptyProductList;
    EXPECT_CALL(*m_fileSystemMock, isDirectory(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_fileSystemMock, listFiles(_)).WillOnce(Return(emptyList));
    ProductUninstaller uninstallManager;

    EXPECT_EQ(uninstallManager.removeProductsNotDownloaded(emptyProductList, m_mockWarehouseRepository).size(), 0);
}

TEST_F( // NOLINT
    ProductUninstallerTest,
    removeProductsNotDownloadedDoesNotAttemptToUninstallProductWhenInstalledProductDirectoryDoesNotExist)
{
    std::vector<std::string> emptyList;
    std::vector<suldownloaderdata::DownloadedProduct> emptyProductList;
    EXPECT_CALL(*m_fileSystemMock, isDirectory(_)).WillOnce(Return(false));
    EXPECT_CALL(*m_fileSystemMock, listFiles(_)).Times(0);
    ProductUninstaller uninstallManager;

    EXPECT_EQ(uninstallManager.removeProductsNotDownloaded(emptyProductList, m_mockWarehouseRepository).size(), 0);
}

TEST_F( // NOLINT
    ProductUninstallerTest,
    removeProductsNotDownloadedDoesNotAttemptToUninstallProductWhenOnlyInstalledProductsInList)
{
    std::vector<std::string> fileList = createDefaultFileList(3);
    std::vector<suldownloaderdata::DownloadedProduct> productList = createDefaultDownloadProductList(3);
    EXPECT_CALL(*m_fileSystemMock, isDirectory(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_fileSystemMock, listFiles(_)).WillOnce(Return(fileList));
    ProductUninstaller uninstallManager;

    EXPECT_EQ(uninstallManager.removeProductsNotDownloaded(productList, m_mockWarehouseRepository).size(), 0);
}

TEST_F( // NOLINT
    ProductUninstallerTest,
    removeProductsNotDownloadedDoesNotAttemptToUninstallProductWhenMoreProductsAreInDownloadListThanInstalled)
{
    std::vector<std::string> fileList = createDefaultFileList(1);
    std::vector<suldownloaderdata::DownloadedProduct> productList = createDefaultDownloadProductList(3);
    EXPECT_CALL(*m_fileSystemMock, isDirectory(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_fileSystemMock, listFiles(_)).WillOnce(Return(fileList));
    ProductUninstaller uninstallManager;

    EXPECT_EQ(uninstallManager.removeProductsNotDownloaded(productList, m_mockWarehouseRepository).size(), 0);
}

TEST_F( // NOLINT
    ProductUninstallerTest,
    removeProductsNotDownloaded_ExpectToUninstallProductWhenProductNotInDownloadListButIsInstalled)
{
    std::vector<std::string> fileList = createDefaultFileList(3);
    std::vector<suldownloaderdata::DownloadedProduct> productList = createDefaultDownloadProductList(2);
    EXPECT_CALL(*m_fileSystemMock, isDirectory(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_fileSystemMock, listFiles(_)).WillOnce(Return(fileList));
    EXPECT_CALL(*m_fileSystemMock, removeFileOrDirectory(_)).Times(1);
    ProductUninstaller uninstallManager;

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&fileList]() {
        auto mockProcess = new StrictMock<MockProcess>();
        EXPECT_CALL(*mockProcess, exec(fileList[2], _, _)).Times(1);
        EXPECT_CALL(*mockProcess, output()).WillOnce(Return("uninstalling product"));
        EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
        return std::unique_ptr<Common::Process::IProcess>(mockProcess);
    });
    EXPECT_CALL(m_mockWarehouseRepository, getProductDistributionPath(_)).WillOnce(Return(m_defaultDistributionPath));
    std::vector<suldownloaderdata::DownloadedProduct> actualProductList;
    EXPECT_NO_THROW(actualProductList = uninstallManager.removeProductsNotDownloaded(
                        productList, m_mockWarehouseRepository);); // NOLINT
    EXPECT_EQ(actualProductList.size(), 1);
    EXPECT_TRUE(actualProductList[0].getProductIsBeingUninstalled());

    EXPECT_EQ(actualProductList[0].getLine(), fileList[2].substr(0, fileList[2].find(".sh")));
}

TEST_F(ProductUninstallerTest, removeProductsNotDownloaded_FailureToUninstallProductThrowsAndIsHandled) // NOLINT
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
    EXPECT_CALL(m_mockWarehouseRepository, getProductDistributionPath(_)).WillOnce(Return(m_defaultDistributionPath));
    std::vector<suldownloaderdata::DownloadedProduct> actualProductList;
    EXPECT_NO_THROW(actualProductList = uninstallManager.removeProductsNotDownloaded(
                        productList, m_mockWarehouseRepository);); // NOLINT
    EXPECT_EQ(actualProductList.size(), 1);
    EXPECT_TRUE(actualProductList[0].getProductIsBeingUninstalled());
    EXPECT_EQ(actualProductList[0].getLine(), fileList[2].substr(0, fileList[2].find(".sh")));
    EXPECT_TRUE(actualProductList[0].getError().Description.find("ProcessThrow") != std::string::npos);
    EXPECT_EQ(actualProductList[0].getError().status, suldownloaderdata::WarehouseStatus::UNINSTALLFAILED);
    EXPECT_EQ(actualProductList[0].getError().SulError, "");
}

TEST_F(ProductUninstallerTest, removeProductsNotDownloaded_FailureToUninstallProductReturnsNonZeroErrorCode) // NOLINT
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
    EXPECT_CALL(m_mockWarehouseRepository, getProductDistributionPath(_)).WillOnce(Return(m_defaultDistributionPath));
    std::vector<suldownloaderdata::DownloadedProduct> actualProductList;
    EXPECT_NO_THROW(actualProductList = uninstallManager.removeProductsNotDownloaded(
                        productList, m_mockWarehouseRepository);); // NOLINT
    EXPECT_EQ(actualProductList.size(), 1);
    EXPECT_TRUE(actualProductList[0].getProductIsBeingUninstalled());
    EXPECT_EQ(actualProductList[0].getLine(), fileList[2].substr(0, fileList[2].find(".sh")));
    EXPECT_TRUE(
        actualProductList[0].getError().Description.find("Process did not complete successfully") != std::string::npos);
    EXPECT_EQ(actualProductList[0].getError().status, suldownloaderdata::WarehouseStatus::UNINSTALLFAILED);
    EXPECT_EQ(actualProductList[0].getError().SulError, "");
}

TEST_F( // NOLINT
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
    EXPECT_CALL(m_mockWarehouseRepository, getProductDistributionPath(_)).WillOnce(Return("/"));

    EXPECT_THROW(
        uninstallManager.removeProductsNotDownloaded(productList, m_mockWarehouseRepository),
        std::logic_error); // NOLINT
}
