/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <SulDownloader/ProductUninstaller.h>
#include <tests/Common/FileSystemImpl/MockFileSystem.h>
#include <modules/Common/FileSystemImpl/FileSystemImpl.h>
#include <modules/Common/ProcessImpl/ProcessImpl.h>
#include <tests/Common/ProcessImpl/MockProcess.h>
#include <modules/Common/Process/IProcessException.h>

using namespace SulDownloader;

class ProductUninstallerTest : public ::testing::Test
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
    std::vector<DownloadedProduct> createDefaultDownloadProductList(int numberProducts)
    {
        std::vector<DownloadedProduct> productList;

        for(int i = 1; i <= numberProducts; i++)
        {
            std::stringstream productLine;
            productLine << "product" << i;
            ProductMetadata metadata;
            metadata.setLine(productLine.str());
            DownloadedProduct product(metadata);
            productList.push_back(product);
        }

        return productList;
    }

    std::vector<std::string> createDefaultFileList(int numberProducts)
    {
        std::vector<std::string> fileList;

        for(int i = 1; i <= numberProducts; i++)
        {
            std::stringstream productLine;
            productLine << "product" << i << ".sh";
            fileList.push_back(productLine.str());
        }

        return fileList;
    }

    MockFileSystem* m_fileSystemMock;

};

TEST_F(ProductUninstallerTest, defaultConstructorDoesNotThrow)
{
    EXPECT_NO_THROW(ProductUninstaller uninstallManager;);
}

TEST_F(ProductUninstallerTest, removeProductsNotDownloadedDoesNotAttemptToUninstallProductWhenNoProductsInstalled)
{
    std::vector<std::string> emptyList;
    std::vector<DownloadedProduct> emptyProductList;
    EXPECT_CALL(*m_fileSystemMock,isDirectory(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_fileSystemMock, listFiles(_)).WillOnce(Return(emptyList));
    ProductUninstaller uninstallManager;

    EXPECT_EQ(uninstallManager.removeProductsNotDownloaded(emptyProductList).size(), 0);
}

TEST_F(ProductUninstallerTest, removeProductsNotDownloadedDoesNotAttemptToUninstallProductWhenInstalledProductDirectoryDoesNotExist)
{
    std::vector<std::string> emptyList;
    std::vector<DownloadedProduct> emptyProductList;
    EXPECT_CALL(*m_fileSystemMock,isDirectory(_)).WillOnce(Return(false));
    EXPECT_CALL(*m_fileSystemMock, listFiles(_)).Times(0);
    ProductUninstaller uninstallManager;

    EXPECT_EQ(uninstallManager.removeProductsNotDownloaded(emptyProductList).size(), 0);
}

TEST_F(ProductUninstallerTest, removeProductsNotDownloadedDoesNotAttemptToUninstallProductWhenOnlyInstalledProductsInList)
{
    std::vector<std::string> fileList = createDefaultFileList(3);
    std::vector<DownloadedProduct> productList = createDefaultDownloadProductList(3);
    EXPECT_CALL(*m_fileSystemMock,isDirectory(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_fileSystemMock, listFiles(_)).WillOnce(Return(fileList));
    ProductUninstaller uninstallManager;

    EXPECT_EQ(uninstallManager.removeProductsNotDownloaded(productList).size(), 0);
}

TEST_F(ProductUninstallerTest, removeProductsNotDownloadedDoesNotAttemptToUninstallProductWhenMoreProductsAreInDownloadListThanInstalled)
{
    std::vector<std::string> fileList = createDefaultFileList(1);
    std::vector<DownloadedProduct> productList = createDefaultDownloadProductList(3);
    EXPECT_CALL(*m_fileSystemMock,isDirectory(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_fileSystemMock, listFiles(_)).WillOnce(Return(fileList));
    ProductUninstaller uninstallManager;

    EXPECT_EQ(uninstallManager.removeProductsNotDownloaded(productList).size(), 0);
}


TEST_F(ProductUninstallerTest, removeProductsNotDownloaded_ExpectToUninstallProductWhenProductNotInDownloadListButIsInstalled)
{
    std::vector<std::string> fileList = createDefaultFileList(3);
    std::vector<DownloadedProduct> productList = createDefaultDownloadProductList(2);
    EXPECT_CALL(*m_fileSystemMock,isDirectory(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_fileSystemMock, listFiles(_)).WillOnce(Return(fileList));
    ProductUninstaller uninstallManager;

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&fileList]() {
                                                                       auto mockProcess = new MockProcess();
                                                                       EXPECT_CALL(*mockProcess, exec(fileList[2], _, _)).Times(1);
                                                                       EXPECT_CALL(*mockProcess, output()).WillOnce(Return("uninstalling product"));
                                                                       EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
                                                                       return std::unique_ptr<Common::Process::IProcess>(mockProcess);
                                                                   }
    );

    std::vector<DownloadedProduct> actualProductList;
    EXPECT_NO_THROW(actualProductList = uninstallManager.removeProductsNotDownloaded(productList););
    EXPECT_EQ(actualProductList.size(), 1);
    EXPECT_TRUE(actualProductList[0].getProductIsBeingUninstalled());

    EXPECT_EQ(actualProductList[0].getLine(), fileList[2].substr(0, fileList[2].find(".sh")));
}


TEST_F(ProductUninstallerTest, removeProductsNotDownloaded_FailureToUninstallProductThrowsAndIsHandled)
{
    std::vector<std::string> fileList = createDefaultFileList(3);
    std::vector<DownloadedProduct> productList = createDefaultDownloadProductList(2);
    EXPECT_CALL(*m_fileSystemMock,isDirectory(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_fileSystemMock, listFiles(_)).WillOnce(Return(fileList));
    ProductUninstaller uninstallManager;

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&fileList]() {
                                                                       auto mockProcess = new MockProcess();
                                                                       EXPECT_CALL(*mockProcess, exec(fileList[2], _, _)).WillOnce(Throw(Common::Process::IProcessException("ProcessThrow")));
                                                                       return std::unique_ptr<Common::Process::IProcess>(mockProcess);
                                                                   }
    );

    std::vector<DownloadedProduct> actualProductList;
    EXPECT_NO_THROW(actualProductList = uninstallManager.removeProductsNotDownloaded(productList););
    EXPECT_EQ(actualProductList.size(), 1);
    EXPECT_TRUE(actualProductList[0].getProductIsBeingUninstalled());
    EXPECT_EQ(actualProductList[0].getLine(), fileList[2].substr(0, fileList[2].find(".sh")));
    EXPECT_TRUE(actualProductList[0].getError().Description.find("ProcessThrow") != std::string::npos);
    EXPECT_EQ(actualProductList[0].getError().status, WarehouseStatus::UNINSTALLFAILED);
    EXPECT_EQ(actualProductList[0].getError().SulError, "");
}

TEST_F(ProductUninstallerTest, removeProductsNotDownloaded_FailureToUninstallProductReturnsNonZeroErrorCode)
{
    std::vector<std::string> fileList = createDefaultFileList(3);
    std::vector<DownloadedProduct> productList = createDefaultDownloadProductList(2);
    EXPECT_CALL(*m_fileSystemMock,isDirectory(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_fileSystemMock, listFiles(_)).WillOnce(Return(fileList));
    ProductUninstaller uninstallManager;

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&fileList]() {
                                                                       auto mockProcess = new MockProcess();
                                                                       EXPECT_CALL(*mockProcess, exec(fileList[2], _, _)).Times(1);
                                                                       EXPECT_CALL(*mockProcess, output()).WillOnce(Return("uninstalling product failed"));
                                                                       EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(1));
                                                                       return std::unique_ptr<Common::Process::IProcess>(mockProcess);
                                                                   }
    );

    std::vector<DownloadedProduct> actualProductList;
    EXPECT_NO_THROW(actualProductList = uninstallManager.removeProductsNotDownloaded(productList););
    EXPECT_EQ(actualProductList.size(), 1);
    EXPECT_TRUE(actualProductList[0].getProductIsBeingUninstalled());
    EXPECT_EQ(actualProductList[0].getLine(), fileList[2].substr(0, fileList[2].find(".sh")));
    EXPECT_TRUE(actualProductList[0].getError().Description.find("Process did not complete successfully") != std::string::npos);
    EXPECT_EQ(actualProductList[0].getError().status, WarehouseStatus::UNINSTALLFAILED);
    EXPECT_EQ(actualProductList[0].getError().SulError, "");
}