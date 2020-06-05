/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

/**
 * Component tests to SULDownloader mocking out WarehouseRepository
 */

#include "ConfigurationSettings.pb.h"
#include "MockVersig.h"
#include "MockWarehouseRepository.h"
#include "TestWarehouseHelper.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/FileSystemImpl/FileSystemImpl.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/ProcessImpl/ArgcAndEnv.h>
#include <Common/ProcessImpl/ProcessImpl.h>
#include <Common/ProtobufUtil/MessageUtility.h>
#include <Common/UtilityImpl/ProjectNames.h>
#include <SulDownloader/SulDownloader.h>
#include <SulDownloader/suldownloaderdata/ConfigurationData.h>
#include <SulDownloader/suldownloaderdata/DownloadReport.h>
#include <SulDownloader/suldownloaderdata/SulDownloaderException.h>
#include <SulDownloader/suldownloaderdata/TimeTracker.h>
#include <SulDownloader/suldownloaderdata/VersigImpl.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tests/Common/FileSystemImpl/MockPidLockFileUtils.h>
#include <tests/Common/Helpers/FilePermissionsReplaceAndRestore.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/MockFilePermissions.h>
#include <tests/Common/Helpers/MockFileSystem.h>
#include <tests/Common/ProcessImpl/MockProcess.h>

using namespace SulDownloader::suldownloaderdata;
using SulDownloaderProto::ConfigurationSettings;

struct SimplifiedDownloadReport
{
    SulDownloader::suldownloaderdata::WarehouseStatus Status;
    std::string Description;
    std::vector<ProductReport> Products;
    bool shouldContainSyncTime;
    std::vector<ProductInfo> WarehouseComponents;
};

namespace
{
    using DownloadedProductVector = std::vector<SulDownloader::suldownloaderdata::DownloadedProduct>;
    using ProductReportVector = std::vector<SulDownloader::suldownloaderdata::ProductReport>;
} // namespace

class SULDownloaderTest : public ::testing::Test
{
    Common::Logging::ConsoleLoggingSetup m_consoleLogging;

public:
    /**
     * Setup directories and files expected by SULDownloader to enable its execution.
     * Use TempDir
     */
    void SetUp() override
    {
        Test::SetUp();

        SulDownloader::suldownloaderdata::VersigFactory::instance().replaceCreator([]() {
            auto versig = new NiceMock<MockVersig>();
            ON_CALL(*versig, verify(_, _))
                .WillByDefault(Return(SulDownloader::suldownloaderdata::IVersig::VerifySignature::SIGNATURE_VERIFIED));
            return std::unique_ptr<SulDownloader::suldownloaderdata::IVersig>(versig);
        });
        m_mockptr = nullptr;

        auto mockPidLockFileUtilsPtr = new NiceMock<MockPidLockFileUtils>();
        ON_CALL(*mockPidLockFileUtilsPtr, write(_, _, _)).WillByDefault(Return(1));
        std::unique_ptr<MockPidLockFileUtils> mockPidLockFileUtils =
            std::unique_ptr<MockPidLockFileUtils>(mockPidLockFileUtilsPtr);
        Common::FileSystemImpl::replacePidLockUtils(std::move(mockPidLockFileUtils));
    }

    /**
     * Remove the temporary directory.
     */
    void TearDown() override
    {
        Tests::restoreFileSystem();
        Common::FileSystemImpl::restorePidLockUtils();
        SulDownloader::suldownloaderdata::VersigFactory::instance().restoreCreator();
        TestWarehouseHelper helper;
        helper.restoreWarehouseFactory();
        Test::TearDown();
    }

    ConfigurationSettings defaultSettings()
    {
        ConfigurationSettings settings;
        Credentials credentials;

        settings.add_sophosurls("http://ostia.eng.sophosinvalid/latest/Virt-vShieldInvalid");
        // settings.add_updatecache("http://192.168.10.10:800/latest/Virt-vShieldInvalid");
        settings.mutable_credential()->set_username("administrator");
        settings.mutable_credential()->set_password("password");
        settings.mutable_proxy()->set_url("noproxy:");
        settings.mutable_proxy()->mutable_credential()->set_username("");
        settings.mutable_proxy()->mutable_credential()->set_password("");
        auto proto_subscription = settings.mutable_primarysubscription();
        proto_subscription->set_rigidname("Everest-Base");
        proto_subscription->set_baseversion("");
        proto_subscription->set_tag("RECOMMMENDED");
        proto_subscription->set_fixedversion("");
        settings.add_features("CORE");
        settings.set_certificatepath("/installroot/base/update/certificates");
        settings.set_installationrootpath("/installroot");
        settings.set_systemsslpath("/installroot/etc/ssl");
        settings.set_cacheupdatesslpath("/installroot/etc/cache/ssl");

        return settings;
    }


    ConfigurationSettings newFeatureSettings()
    {
        ConfigurationSettings settings = defaultSettings();
        settings.add_features("SOME_FEATURE");
        return settings;
    }

    ConfigurationSettings newDifferentFeatureSettings()
    {
        ConfigurationSettings settings = defaultSettings();
        settings.add_features("DIFF_FEATURE");
        return settings;
    }


    ConfigurationSettings newSubscriptionSettings()
    {
        ConfigurationSettings settings = defaultSettings();
        auto proto_subscriptions = settings.mutable_products();

        SulDownloaderProto::ConfigurationSettings_Subscription proto_subscription;

        proto_subscription.set_rigidname("Plugin_1");
        proto_subscription.set_baseversion("");
        proto_subscription.set_tag("RECOMMMENDED");
        proto_subscription.set_fixedversion("");


        proto_subscriptions->Add(
                dynamic_cast<SulDownloaderProto::ConfigurationSettings_Subscription &&>(proto_subscription));

        return settings;
    }

    ConfigurationSettings newDifferentSubscriptionSettings()
    {
        ConfigurationSettings settings = defaultSettings();
        auto proto_subscriptions = settings.mutable_products();

        SulDownloaderProto::ConfigurationSettings_Subscription proto_subscription;

        proto_subscription.set_rigidname("Plugin_2");
        proto_subscription.set_baseversion("");
        proto_subscription.set_tag("RECOMMMENDED");
        proto_subscription.set_fixedversion("");


        proto_subscriptions->Add(
                dynamic_cast<SulDownloaderProto::ConfigurationSettings_Subscription &&>(proto_subscription));

        return settings;
    }





    std::string jsonSettings(const ConfigurationSettings& configSettings)
    {
        return Common::ProtobufUtil::MessageUtility::protoBuf2Json(configSettings);
    }

    SulDownloader::suldownloaderdata::ConfigurationData configData(const ConfigurationSettings& configSettings)
    {
        std::string json_output = jsonSettings(configSettings);
        return SulDownloader::suldownloaderdata::ConfigurationData::fromJsonSettings(json_output);
    }

    DownloadedProductVector defaultProducts()
    {
        DownloadedProductVector products;
        for (auto& metadata : defaultMetadata())
        {
            SulDownloader::suldownloaderdata::DownloadedProduct product(metadata);
            product.setDistributePath("/installroot/base/update/cache/primary");
            products.push_back(product);
        }
        return products;
    }

    std::vector<suldownloaderdata::ProductInfo> productsInfo(const DownloadedProductVector& products)
    {
        std::vector<suldownloaderdata::ProductInfo> info;
        for (auto& product : products)
        {
            ProductInfo pInfo;
            ProductMetadata metadata = product.getProductMetadata();
            pInfo.m_rigidName = metadata.getLine();
            pInfo.m_productName = metadata.getName();
            pInfo.m_version = metadata.getVersion();
            info.push_back(pInfo);
        }
        return info;
    }

    std::vector<suldownloaderdata::SubscriptionInfo> subscriptionsInfo(const DownloadedProductVector& products)
    {
        std::vector<suldownloaderdata::SubscriptionInfo> subInfos;
        for (auto& product : products)
        {
            SubscriptionInfo pInfo;
            ProductMetadata metadata = product.getProductMetadata();
            pInfo.rigidName = metadata.getLine();
            pInfo.version = metadata.getVersion();
            subInfos.push_back(pInfo);
        }
        return subInfos;
    }


    std::vector<SulDownloader::suldownloaderdata::ProductMetadata> defaultMetadata()
    {
        SulDownloader::suldownloaderdata::ProductMetadata base;
        base.setDefaultHomePath("everest");
        base.setLine("Everest-Base");
        base.setName("Everest-Base-Product");
        base.setVersion("10.2.3");
        base.setTags({ { "RECOMMENDED", "10", "Base-label" } });

        SulDownloader::suldownloaderdata::ProductMetadata plugin;
        plugin.setDefaultHomePath("everest-plugin-a");
        plugin.setLine("Everest-Plugins-A");
        plugin.setName("Everest-Plugins-A-Product");
        plugin.setVersion("10.3.5");
        plugin.setTags({ { "RECOMMENDED", "10", "Plugin-label" } });

        return std::vector<SulDownloader::suldownloaderdata::ProductMetadata>{ base, plugin };
    }

    ProductReportVector defaultProductReports()
    {
        SulDownloader::suldownloaderdata::ProductReport base;
        base.name = "Everest-Base-Product";
        base.rigidName = "Everest-Base";
        base.downloadedVersion = "10.2.3";
        base.productStatus = ProductReport::ProductStatus::UpToDate;
        SulDownloader::suldownloaderdata::ProductReport plugin;
        plugin.name = "Everest-Plugins-A-Product";
        plugin.rigidName = "Everest-Plugins-A";
        plugin.downloadedVersion = "10.3.5";
        plugin.productStatus = ProductReport::ProductStatus::UpToDate;
        return ProductReportVector{ base, plugin };
    }

    MockWarehouseRepository& warehouseMocked()
    {
        if (m_mockptr == nullptr)
        {
            m_mockptr = new StrictMock<MockWarehouseRepository>();
            TestWarehouseHelper helper;
            helper.replaceWarehouseCreator([this](const suldownloaderdata::ConfigurationData&) {
                return suldownloaderdata::IWarehouseRepositoryPtr(this->m_mockptr);
            });
        }
        return *m_mockptr;
    }

    MockFileSystem& setupFileSystemAndGetMock(int expectCallCount=1)
    {
        auto filesystemMock = new StrictMock<MockFileSystem>();
        EXPECT_CALL(*filesystemMock, isDirectory("/installroot")).Times(expectCallCount).WillRepeatedly(Return(true));
        EXPECT_CALL(*filesystemMock, isDirectory("/installroot/base/update/cache/primarywarehouse"))
            .Times(expectCallCount).WillRepeatedly(Return(true));
        EXPECT_CALL(*filesystemMock, isDirectory("/installroot/base/update/cache/primary")).Times(expectCallCount).WillRepeatedly(Return(true));
        EXPECT_CALL(*filesystemMock, exists(_)).WillRepeatedly(Return(true));

        auto pointer = filesystemMock;
        Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));
        return *pointer;
    }

    void setupExpectanceWriteAtomically(MockFileSystem& mockFileSystem, const std::string& contains)
    {
        EXPECT_CALL(
            mockFileSystem, writeFile(::testing::HasSubstr("/installroot/tmp"), ::testing::HasSubstr(contains)));
        EXPECT_CALL(mockFileSystem, moveFile(_, "/dir/output.json"));
        auto mockFilePermissions = new StrictMock<MockFilePermissions>();
        EXPECT_CALL(*mockFilePermissions, chown(_, sophos::user(), sophos::group()));
        std::unique_ptr<MockFilePermissions> mockIFilePermissionsPtr =
            std::unique_ptr<MockFilePermissions>(mockFilePermissions);
        Tests::replaceFilePermissions(std::move(mockIFilePermissionsPtr));
    }

    ::testing::AssertionResult downloadReportSimilar(
        const char* m_expr,
        const char* n_expr,
        const SimplifiedDownloadReport& expected,
        const SulDownloader::suldownloaderdata::DownloadReport& resulted)
    {
        std::stringstream s;
        s << m_expr << " and " << n_expr << " failed: ";

        if (expected.Status != resulted.getStatus())
        {
            return ::testing::AssertionFailure()
                   << s.str()
                   << " status differ: \n expected: " << SulDownloader::suldownloaderdata::toString(expected.Status)
                   << "\n result: " << SulDownloader::suldownloaderdata::toString(resulted.getStatus());
        }
        if (expected.Description != resulted.getDescription())
        {
            return ::testing::AssertionFailure()
                   << s.str() << " Description differ: \n expected: " << expected.Description
                   << "\n result: " << resulted.getDescription();
        }

        std::string expectedProductsSerialized = productsToString(expected.Products);
        std::string resultedProductsSerialized = productsToString(resulted.getProducts());
        if (expectedProductsSerialized != resultedProductsSerialized)
        {
            return ::testing::AssertionFailure()
                   << s.str() << " Different products. \n expected: " << expectedProductsSerialized
                   << "\n result: " << resultedProductsSerialized;
        }

        if (expected.shouldContainSyncTime)
        {
            if (resulted.getSyncTime().empty())
            {
                return ::testing::AssertionFailure() << s.str() << " Expected SyncTime not empty. Found it empty. ";
            }
        }
        else
        {
            if (!resulted.getSyncTime().empty())
            {
                return ::testing::AssertionFailure()
                       << s.str() << "Expected SyncTime to be empty, but found it: " << resulted.getSyncTime();
            }
        }

        return listProductInfoIsEquivalent(
            m_expr, n_expr, expected.WarehouseComponents, resulted.getWarehouseComponents());
    }

    std::string productsToString(const ProductReportVector& productsReport)
    {
        std::stringstream stream;
        for (auto& productReport : productsReport)
        {
            stream << "name: " << productReport.name << ", version: " << productReport.downloadedVersion
                   << ", error: " << productReport.errorDescription
                   << ", productstatus: " << productReport.statusToString() << '\n';
        }
        std::string serialized = stream.str();
        if (serialized.empty())
        {
            return "{empty}";
        }
        return serialized;
    }

protected:
    MockWarehouseRepository* m_mockptr = nullptr;
};

TEST_F(SULDownloaderTest, configurationDataVerificationOfDefaultSettingsReturnsTrue) // NOLINT
{
    setupFileSystemAndGetMock();
    SulDownloader::suldownloaderdata::ConfigurationData confData = configData(defaultSettings());
    confData.verifySettingsAreValid();
    EXPECT_TRUE(confData.isVerified());
}

TEST_F(SULDownloaderTest, main_entry_InvalidArgumentsReturnsTheCorrectErrorCode) // NOLINT
{
    auto filesystemMock = new MockFileSystem();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    int expectedErrorCode = -2;

    char** argsNotUsed = nullptr;
    EXPECT_EQ(SulDownloader::main_entry(2, argsNotUsed), -1);
    EXPECT_EQ(SulDownloader::main_entry(1, argsNotUsed), -1);
    char* inputFileDoesNotExist[] = { const_cast<char*>("SulDownloader"),
                                      const_cast<char*>("inputfiledoesnotexists.json"),
                                      const_cast<char*>("createoutputpath.json") };
    EXPECT_CALL(*filesystemMock, readFile("inputfiledoesnotexists.json"))
        .WillOnce(Throw(Common::FileSystem::IFileSystemException("")));

    EXPECT_EQ(SulDownloader::main_entry(3, inputFileDoesNotExist), -2);

    EXPECT_CALL(*filesystemMock, readFile("input.json")).WillOnce(Return(jsonSettings(defaultSettings())));

    EXPECT_CALL(*filesystemMock, isDirectory("/installroot/directorypath")).WillOnce(Return(true));
    // directory can not be replaced by file
    Common::ProcessImpl::ArgcAndEnv args("SulDownloader", { "input.json", "/installroot/directorypath" }, {});

    EXPECT_EQ(SulDownloader::main_entry(3, args.argc()), expectedErrorCode);
}

TEST_F( // NOLINT
    SULDownloaderTest,
    main_entry_onSuccessWhileForcingUpdateAsPreviousDownloadReportDoesNotExistCreatesReportContainingExpectedSuccessResult)
{
    auto& fileSystemMock = setupFileSystemAndGetMock();
    MockWarehouseRepository& mock = warehouseMocked();

    auto products = defaultProducts();
    products[0].setProductHasChanged(false);
    products[1].setProductHasChanged(false);

    EXPECT_CALL(mock, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(mock, synchronize(_));
    EXPECT_CALL(mock, distribute());
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/installroot/base/update/cache/primary/everest");
    products[1].setDistributePath("/installroot/base/update/cache/primary/everest-plugin-a");
    EXPECT_CALL(mock, getProducts()).WillOnce(Return(products));
    EXPECT_CALL(mock, getSourceURL());
    EXPECT_CALL(mock, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(mock, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));    

    std::vector<std::string> emptyFileList;
    // it should not depend on currentWorkingDirectory:  	LINUXEP-6153
    EXPECT_CALL(fileSystemMock, currentWorkingDirectory()).Times(0);
    EXPECT_CALL(fileSystemMock, readFile("/dir/input.json")).WillOnce(Return(jsonSettings(defaultSettings())));
    EXPECT_CALL(fileSystemMock, isFile("/dir/previous_update_config.json")).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, isDirectory("/dir/output.json")).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, isDirectory("/dir")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, listFiles("/dir")).WillOnce(Return(emptyFileList));
    setupExpectanceWriteAtomically(
        fileSystemMock,
        SulDownloader::suldownloaderdata::toString(SulDownloader::suldownloaderdata::WarehouseStatus::SUCCESS));
    std::string baseInstallPath = "/installroot/base/update/cache/primary/everest/install.sh";
    EXPECT_CALL(fileSystemMock, isDirectory(baseInstallPath)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(baseInstallPath));

    std::string pluginInstallPath = "/installroot/base/update/cache/primary/everest-plugin-a/install.sh";
    EXPECT_CALL(fileSystemMock, isDirectory(pluginInstallPath)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(pluginInstallPath));

    std::string uninstallPath = "/installroot/base/update/var/installedproducts";
    EXPECT_CALL(fileSystemMock, isDirectory(uninstallPath)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, listFiles(uninstallPath)).WillOnce(Return(emptyFileList));

    Common::ProcessImpl::ArgcAndEnv args("SulDownloader", { "/dir/input.json", "/dir/output.json" }, {});

    int counter = 0;
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&counter]() {
        if (counter++ == 0)
        {
            auto mockProcess = new StrictMock<MockProcess>();
            EXPECT_CALL(*mockProcess, exec(HasSubstr("everest/install.sh"), _, _)).Times(1);
            EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, output()).WillOnce(Return("installing base"));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        }
        else
        {
            auto mockProcess = new StrictMock<MockProcess>();
            EXPECT_CALL(*mockProcess, exec(HasSubstr("everest-plugin-a/install.sh"), _, _)).Times(1);
            EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, output()).WillOnce(Return("installing plugin"));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        }
    });

    EXPECT_EQ(SulDownloader::main_entry(3, args.argc()), 0);
}

TEST_F(SULDownloaderTest, main_entry_onSuccessCreatesReportContainingExpectedSuccessResult) // NOLINT
{
    auto& fileSystemMock = setupFileSystemAndGetMock();
    MockWarehouseRepository& mock = warehouseMocked();

    auto products = defaultProducts();
    products[0].setProductHasChanged(false);
    products[1].setProductHasChanged(false);

    EXPECT_CALL(mock, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(mock, synchronize(_));
    EXPECT_CALL(mock, distribute());
    EXPECT_CALL(mock, getProducts()).WillOnce(Return(products));
    EXPECT_CALL(mock, getSourceURL());
    EXPECT_CALL(mock, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(mock, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));    

    TimeTracker timeTracker;
    timeTracker.setStartTime(std::time_t(0));
    timeTracker.setFinishedTime(std::time_t(0));
    DownloadReport downloadReport =
        DownloadReport::Report("", products, {}, {}, &timeTracker, DownloadReport::VerifyState::VerifyCorrect);
    std::string previousJsonReport = DownloadReport::fromReport(downloadReport);
    std::string previousReportFilename = "update_report-previous.json";
    std::vector<std::string> previousReportFileList = { previousReportFilename };
    std::vector<std::string> emptyFileList;
    // it should not depend on currentWorkingDirectory:  	LINUXEP-6153
    EXPECT_CALL(fileSystemMock, currentWorkingDirectory()).Times(0);
    EXPECT_CALL(fileSystemMock, readFile("/dir/input.json")).WillOnce(Return(jsonSettings(defaultSettings())));
    EXPECT_CALL(fileSystemMock, isFile("/dir/previous_update_config.json")).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, isDirectory("/dir/output.json")).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, isDirectory("/dir")).WillOnce(Return(true));

    EXPECT_CALL(fileSystemMock, listFiles("/dir")).WillOnce(Return(previousReportFileList));
    EXPECT_CALL(fileSystemMock, readFile(previousReportFilename)).WillOnce(Return(previousJsonReport));

    setupExpectanceWriteAtomically(
        fileSystemMock,
        SulDownloader::suldownloaderdata::toString(SulDownloader::suldownloaderdata::WarehouseStatus::SUCCESS));

    std::string uninstallPath = "/installroot/base/update/var/installedproducts";
    EXPECT_CALL(fileSystemMock, isDirectory(uninstallPath)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, listFiles(uninstallPath)).WillOnce(Return(emptyFileList));

    Common::ProcessImpl::ArgcAndEnv args("SulDownloader", { "/dir/input.json", "/dir/output.json" }, {});

    EXPECT_EQ(SulDownloader::main_entry(3, args.argc()), 0);
}

TEST_F( // NOLINT
    SULDownloaderTest,
    main_entry_onSuccessCreatesReportContainingExpectedSuccessResultEnsuringInvalidPreviousReportFilesAreIgnored)
{
    auto& fileSystemMock = setupFileSystemAndGetMock();
    MockWarehouseRepository& mock = warehouseMocked();

    auto products = defaultProducts();
    products[0].setProductHasChanged(false);
    products[1].setProductHasChanged(false);

    EXPECT_CALL(mock, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(mock, synchronize(_));
    EXPECT_CALL(mock, distribute());
    EXPECT_CALL(mock, getProducts()).WillOnce(Return(products));
    EXPECT_CALL(mock, getSourceURL());
    EXPECT_CALL(mock, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(mock, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));    

    TimeTracker timeTracker;
    timeTracker.setStartTime(std::time_t(0));
    timeTracker.setFinishedTime(std::time_t(0));
    DownloadReport downloadReport =
        DownloadReport::Report("", products, {}, {}, &timeTracker, DownloadReport::VerifyState::VerifyCorrect);
    std::string previousJsonReport = DownloadReport::fromReport(downloadReport);
    std::string previousReportFilename = "update_report-previous.json";
    std::vector<std::string> previousReportFileList = {
        previousReportFilename, "invalid_file_name1.txt", "invalid_file_name2.json", "report_invalid_file_name3.txt"
    };
    std::vector<std::string> emptyFileList;
    // it should not depend on currentWorkingDirectory:  	LINUXEP-6153
    EXPECT_CALL(fileSystemMock, currentWorkingDirectory()).Times(0);
    EXPECT_CALL(fileSystemMock, readFile("/dir/input.json")).WillOnce(Return(jsonSettings(defaultSettings())));
    EXPECT_CALL(fileSystemMock, isFile("/dir/previous_update_config.json")).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, isDirectory("/dir/output.json")).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, isDirectory("/dir")).WillOnce(Return(true));

    EXPECT_CALL(fileSystemMock, listFiles("/dir")).WillOnce(Return(previousReportFileList));
    EXPECT_CALL(fileSystemMock, readFile(previousReportFilename)).WillOnce(Return(previousJsonReport));

    setupExpectanceWriteAtomically(
        fileSystemMock,
        SulDownloader::suldownloaderdata::toString(SulDownloader::suldownloaderdata::WarehouseStatus::SUCCESS));

    std::string uninstallPath = "/installroot/base/update/var/installedproducts";
    EXPECT_CALL(fileSystemMock, isDirectory(uninstallPath)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, listFiles(uninstallPath)).WillOnce(Return(emptyFileList));

    Common::ProcessImpl::ArgcAndEnv args("SulDownloader", { "/dir/input.json", "/dir/output.json" }, {});

    EXPECT_EQ(SulDownloader::main_entry(3, args.argc()), 0);
}

MATCHER_P(isProduct, pname, "")
{
    *result_listener << "whose getLine() is " << arg.getLine();
    return (arg.getLine() == pname);
}

TEST_F( // NOLINT
    SULDownloaderTest,
    main_entry_onSuccessCreatesReportContainingExpectedSuccessResultAndRemovesProduct)
{
    auto& fileSystemMock = setupFileSystemAndGetMock();
    MockWarehouseRepository& mock = warehouseMocked();

    auto products = defaultProducts();
    products[0].setProductHasChanged(false);
    products[1].setProductHasChanged(false);

    EXPECT_CALL(mock, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(mock, synchronize(_));
    EXPECT_CALL(mock, distribute());
    EXPECT_CALL(mock, getProducts()).WillOnce(Return(products));
    EXPECT_CALL(mock, getSourceURL());
    EXPECT_CALL(mock, getProductDistributionPath(isProduct("productRemove1")))
        .WillOnce(Return("/installroot/base/update/cache/primary/productRemove1"));
    EXPECT_CALL(mock, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(mock, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));    

    TimeTracker timeTracker;
    timeTracker.setStartTime(std::time_t(0));
    timeTracker.setFinishedTime(std::time_t(0));
    DownloadReport downloadReport =
        DownloadReport::Report("", products, {}, {}, &timeTracker, DownloadReport::VerifyState::VerifyCorrect);
    std::string previousJsonReport = DownloadReport::fromReport(downloadReport);
    std::string previousReportFilename = "update_report-previous.json";
    std::vector<std::string> previousReportFileList = { previousReportFilename };
    std::vector<std::string> emptyFileList;

    // it should not depend on currentWorkingDirectory:  	LINUXEP-6153
    EXPECT_CALL(fileSystemMock, currentWorkingDirectory()).Times(0);
    EXPECT_CALL(fileSystemMock, readFile("/dir/input.json")).WillOnce(Return(jsonSettings(defaultSettings())));
    EXPECT_CALL(fileSystemMock, isFile("/dir/previous_update_config.json")).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, isDirectory("/dir/output.json")).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, isDirectory("/dir")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, removeFileOrDirectory(_)).Times(1);

    EXPECT_CALL(fileSystemMock, listFiles("/dir")).WillOnce(Return(previousReportFileList));
    EXPECT_CALL(fileSystemMock, readFile(previousReportFilename)).WillOnce(Return(previousJsonReport));

    setupExpectanceWriteAtomically(
        fileSystemMock,
        SulDownloader::suldownloaderdata::toString(SulDownloader::suldownloaderdata::WarehouseStatus::SUCCESS));

    std::vector<std::string> fileListOfProductsToRemove = { "productRemove1" };
    std::string uninstallPath = "/installroot/base/update/var/installedproducts";
    EXPECT_CALL(fileSystemMock, isDirectory(uninstallPath)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, listFiles(uninstallPath)).WillOnce(Return(fileListOfProductsToRemove));

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([]() {
        auto mockProcess = new StrictMock<MockProcess>();
        EXPECT_CALL(*mockProcess, exec(HasSubstr("productRemove1"), _, _)).Times(1);
        EXPECT_CALL(*mockProcess, output()).WillOnce(Return("uninstalling product"));
        EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
        return std::unique_ptr<Common::Process::IProcess>(mockProcess);
    });

    Common::ProcessImpl::ArgcAndEnv args("SulDownloader", { "/dir/input.json", "/dir/output.json" }, {});

    EXPECT_EQ(SulDownloader::main_entry(3, args.argc()), 0);
}

TEST_F( // NOLINT
    SULDownloaderTest,
    main_entry_onSuccessCreatesReportContainingExpectedUninstallFailedResult)
{
    auto& fileSystemMock = setupFileSystemAndGetMock();
    MockWarehouseRepository& mock = warehouseMocked();

    auto products = defaultProducts();
    products[0].setProductHasChanged(false);
    products[1].setProductHasChanged(false);

    EXPECT_CALL(mock, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(mock, synchronize(_));
    EXPECT_CALL(mock, distribute());
    EXPECT_CALL(mock, getProducts()).WillOnce(Return(products));
    EXPECT_CALL(mock, getSourceURL());
    EXPECT_CALL(mock, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(mock, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));    
    EXPECT_CALL(mock, getProductDistributionPath(isProduct("productRemove1"))).WillOnce(Return("productRemove1"));

    TimeTracker timeTracker;
    timeTracker.setStartTime(std::time_t(0));
    timeTracker.setFinishedTime(std::time_t(0));
    DownloadReport downloadReport =
        DownloadReport::Report("", products, {}, {}, &timeTracker, DownloadReport::VerifyState::VerifyCorrect);
    std::string previousJsonReport = DownloadReport::fromReport(downloadReport);
    std::string previousReportFilename = "update_report-previous.json";
    std::vector<std::string> previousReportFileList = { previousReportFilename };
    std::vector<std::string> emptyFileList;

    // it should not depend on currentWorkingDirectory:  	LINUXEP-6153
    EXPECT_CALL(fileSystemMock, currentWorkingDirectory()).Times(0);
    EXPECT_CALL(fileSystemMock, readFile("/dir/input.json")).WillOnce(Return(jsonSettings(defaultSettings())));
    EXPECT_CALL(fileSystemMock, isFile("/dir/previous_update_config.json")).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, isDirectory("/dir/output.json")).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, isDirectory("/dir")).WillOnce(Return(true));

    EXPECT_CALL(fileSystemMock, listFiles("/dir")).WillOnce(Return(previousReportFileList));
    EXPECT_CALL(fileSystemMock, readFile(previousReportFilename)).WillOnce(Return(previousJsonReport));

    setupExpectanceWriteAtomically(
        fileSystemMock,
        SulDownloader::suldownloaderdata::toString(SulDownloader::suldownloaderdata::WarehouseStatus::UNINSTALLFAILED));

    std::vector<std::string> fileListOfProductsToRemove = { "productRemove1" };
    std::string uninstallPath = "/installroot/base/update/var/installedproducts";
    EXPECT_CALL(fileSystemMock, isDirectory(uninstallPath)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, listFiles(uninstallPath)).WillOnce(Return(fileListOfProductsToRemove));

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([]() {
        auto mockProcess = new StrictMock<MockProcess>();
        EXPECT_CALL(*mockProcess, exec(HasSubstr("productRemove1"), _, _)).Times(1);
        EXPECT_CALL(*mockProcess, output()).WillOnce(Return("uninstalling product"));
        EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(1));
        return std::unique_ptr<Common::Process::IProcess>(mockProcess);
    });

    Common::ProcessImpl::ArgcAndEnv args("SulDownloader", { "/dir/input.json", "/dir/output.json" }, {});

    EXPECT_EQ(
        SulDownloader::main_entry(3, args.argc()), SulDownloader::suldownloaderdata::WarehouseStatus::UNINSTALLFAILED);
}

// the other execution paths were covered in main_entry_* tests.
TEST_F( // NOLINT
    SULDownloaderTest,
    fileEntriesAndRunDownloaderThrowIfCannotCreateOutputFile)
{
    auto filesystemMock = new MockFileSystem();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));
    EXPECT_CALL(*filesystemMock, readFile("/dir/input.json")).WillOnce(Return(jsonSettings(defaultSettings())));
    EXPECT_CALL(*filesystemMock, isDirectory("/dir/path/that/cannot/be/created/output.json")).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, isDirectory("/dir/path/that/cannot/be/created")).WillOnce(Return(false));

    EXPECT_THROW( // NOLINT
        SulDownloader::fileEntriesAndRunDownloader("/dir/input.json", "/dir/path/that/cannot/be/created/output.json"),
        SulDownloader::suldownloaderdata::SulDownloaderException);
}

// configAndRunDownloader
TEST_F( // NOLINT
    SULDownloaderTest,
    configAndRunDownloaderInvalidSettingsReportError_WarehouseStatus_UNSPECIFIED)
{
    auto filesystemMock = new MockFileSystem();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));
    std::string reportContent;
    int exitCode = 0;
    auto settings = defaultSettings();
    settings.clear_sophosurls(); // no sophos urls, the system can not connect to warehouses
    std::string settingsString = jsonSettings(settings);
    std::string previousSettingString("");
    std::string previousReportData;

    std::tie(exitCode, reportContent) = SulDownloader::configAndRunDownloader(settingsString, previousSettingString, previousReportData);

    EXPECT_NE(exitCode, 0);
    EXPECT_THAT(
        reportContent,
        ::testing::Not(::testing::HasSubstr(
            SulDownloader::suldownloaderdata::toString(SulDownloader::suldownloaderdata::WarehouseStatus::SUCCESS))));
    EXPECT_THAT(
        reportContent,
        ::testing::HasSubstr(SulDownloader::suldownloaderdata::toString(
            SulDownloader::suldownloaderdata::WarehouseStatus::UNSPECIFIED)));
}

// runSULDownloader
TEST_F( // NOLINT
    SULDownloaderTest,
    runSULDownloader_WarehouseConnectionFailureShouldCreateValidConnectionFailureReport)
{
    setupFileSystemAndGetMock();
    MockWarehouseRepository& mock = warehouseMocked();
    SulDownloader::suldownloaderdata::WarehouseError wError;
    wError.Description = "Error description";
    wError.status = SulDownloader::suldownloaderdata::WarehouseStatus::CONNECTIONERROR;
    std::string statusError = SulDownloader::suldownloaderdata::toString(wError.status);
    DownloadedProductVector emptyProducts;

    EXPECT_CALL(mock, hasError()).WillOnce(Return(true)).WillRepeatedly(Return(true));
    EXPECT_CALL(mock, getError()).WillOnce(Return(wError));
    EXPECT_CALL(mock, getProducts()).WillOnce(Return(emptyProducts));
    EXPECT_CALL(mock, getSourceURL());

    SimplifiedDownloadReport expectedDownloadReport{ wError.status, wError.Description, {}, false, {} };

    ConfigurationData configurationData = configData(defaultSettings());
    ConfigurationData previousConfigurationData;
    configurationData.verifySettingsAreValid();
    DownloadReport previousDownloadReport = DownloadReport::Report("Not assigned");

    EXPECT_PRED_FORMAT2(
        downloadReportSimilar,
        expectedDownloadReport,
        SulDownloader::runSULDownloader(configurationData, previousConfigurationData, previousDownloadReport));
}

TEST_F( // NOLINT
    SULDownloaderTest,
    runSULDownloader_WarehouseSynchronizationFailureShouldCreateValidSyncronizationFailureReport)
{
    setupFileSystemAndGetMock();
    MockWarehouseRepository& mock = warehouseMocked();
    SulDownloader::suldownloaderdata::WarehouseError wError;
    wError.Description = "Error description";
    wError.status = SulDownloader::suldownloaderdata::WarehouseStatus::PACKAGESOURCEMISSING;
    std::string statusError = SulDownloader::suldownloaderdata::toString(wError.status);
    DownloadedProductVector emptyProducts;

    EXPECT_CALL(mock, hasError())
        .WillOnce(Return(false)) // connection
        .WillOnce(Return(true))  // synchronization
        .WillRepeatedly(Return(true));
    EXPECT_CALL(mock, synchronize(_));
    EXPECT_CALL(mock, getError()).WillOnce(Return(wError));
    EXPECT_CALL(mock, getProducts()).WillOnce(Return(emptyProducts));
    EXPECT_CALL(mock, getSourceURL());

    SimplifiedDownloadReport expectedDownloadReport{ wError.status, wError.Description, {}, false, {} };

    ConfigurationData configurationData = configData(defaultSettings());
    ConfigurationData previousConfigurationData;
    configurationData.verifySettingsAreValid();
    DownloadReport previousDownloadReport = DownloadReport::Report("Not assigned");

    EXPECT_PRED_FORMAT2(
        downloadReportSimilar,
        expectedDownloadReport,
        SulDownloader::runSULDownloader(configurationData, previousConfigurationData, previousDownloadReport));
}

/**
 * Simulate the error in distributing only one of the products
 */
TEST_F(SULDownloaderTest, runSULDownloader_onDistributeFailure) // NOLINT
{
    setupFileSystemAndGetMock();
    MockWarehouseRepository& mock = warehouseMocked();
    SulDownloader::suldownloaderdata::WarehouseError wError;
    wError.Description = "Error description";
    wError.status = SulDownloader::suldownloaderdata::WarehouseStatus::DOWNLOADFAILED;
    std::string statusError = SulDownloader::suldownloaderdata::toString(wError.status);
    DownloadedProductVector products = defaultProducts();

    SulDownloader::suldownloaderdata::WarehouseError productError;
    productError.Description = "Product Error description";
    productError.status = SulDownloader::suldownloaderdata::WarehouseStatus::DOWNLOADFAILED;

    products[0].setError(productError);
    products[1].setError(productError);

    ProductReportVector productReports = defaultProductReports();
    productReports[0].errorDescription = productError.Description;
    productReports[0].productStatus = ProductReport::ProductStatus::SyncFailed;

    productReports[1].errorDescription = productError.Description;
    productReports[1].productStatus = ProductReport::ProductStatus::SyncFailed;

    EXPECT_CALL(mock, hasError())
        .WillOnce(Return(false)) // connection
        .WillOnce(Return(false)) // synchronization
        .WillOnce(Return(true))  // distribute
        .WillRepeatedly(Return(true));
    EXPECT_CALL(mock, synchronize(_));
    EXPECT_CALL(mock, distribute());
    EXPECT_CALL(mock, getError()).WillOnce(Return(wError));
    EXPECT_CALL(mock, getProducts()).WillOnce(Return(products));
    EXPECT_CALL(mock, getSourceURL());

    SimplifiedDownloadReport expectedDownloadReport{ wError.status, wError.Description, productReports, false, {} };

    ConfigurationData configurationData = configData(defaultSettings());
    ConfigurationData previousConfigurationData;
    configurationData.verifySettingsAreValid();
    DownloadReport previousDownloadReport = DownloadReport::Report("Not assigned");

    EXPECT_PRED_FORMAT2(
        downloadReportSimilar,
        expectedDownloadReport,
        SulDownloader::runSULDownloader(configurationData, previousConfigurationData, previousDownloadReport));
}

TEST_F( // NOLINT
    SULDownloaderTest,
    runSULDownloader_WarehouseSynchronizationResultingInNoUpdateNeededShouldCreateValidSuccessReport)
{
    auto& fileSystemMock = setupFileSystemAndGetMock();
    MockWarehouseRepository& mock = warehouseMocked();
    DownloadedProductVector products = defaultProducts();
    ProductReportVector productReports = defaultProductReports();

    for (auto& product : products)
    {
        product.setProductHasChanged(false);
    }

    EXPECT_CALL(mock, hasError()).WillRepeatedly(Return(false));

    EXPECT_CALL(mock, synchronize(_));
    EXPECT_CALL(mock, distribute());
    EXPECT_CALL(mock, getProducts()).WillOnce(Return(products));
    EXPECT_CALL(mock, getSourceURL());
    EXPECT_CALL(mock, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(mock, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));    

    std::vector<std::string> emptyFileList;
    std::string uninstallPath = "/installroot/base/update/var/installedproducts";
    EXPECT_CALL(fileSystemMock, isDirectory(uninstallPath)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, listFiles(uninstallPath)).WillOnce(Return(emptyFileList));

    SimplifiedDownloadReport expectedDownloadReport{ SulDownloader::suldownloaderdata::WarehouseStatus::SUCCESS,
                                                     "",
                                                     productReports,
                                                     true,
                                                     productsInfo({ products[0], products[1] }) };

    ConfigurationData configurationData = configData(defaultSettings());
    ConfigurationData previousConfigurationData;
    configurationData.verifySettingsAreValid();

    TimeTracker timeTracker;
    timeTracker.setStartTime(std::time_t(0));
    timeTracker.setFinishedTime(std::time_t(0));
    DownloadReport previousDownloadReport =
        DownloadReport::Report("", products, {}, {}, &timeTracker, DownloadReport::VerifyState::VerifyCorrect);

    EXPECT_PRED_FORMAT2(
        downloadReportSimilar,
        expectedDownloadReport,
        SulDownloader::runSULDownloader(configurationData, previousConfigurationData, previousDownloadReport));
}
/**
 * Simulate invalid signature
 *
 */
TEST_F( // NOLINT
    SULDownloaderTest,
    runSULDownloader_UpdateFailForInvalidSignature)
{
    setupFileSystemAndGetMock();
    MockWarehouseRepository& mock = warehouseMocked();

    ConfigurationData configurationData = configData(defaultSettings());
    ConfigurationData previousConfigurationData;
    configurationData.verifySettingsAreValid();

    DownloadedProductVector products = defaultProducts();

    for (auto& product : products)
    {
        product.setProductHasChanged(true);
    }

    std::string everest_installer = "/installroot/base/update/cache/primary/everest/install.sh";
    std::string plugin_installer = "/installroot/base/update/cache/primary/everest-plugin-a/install.sh";
    int counter = 0;

    SulDownloader::suldownloaderdata::VersigFactory::instance().replaceCreator([&counter]() {
        auto versig = new StrictMock<MockVersig>();
        if (counter++ == 0)
        {
            EXPECT_CALL(*versig, verify(_, "/installroot/base/update/cache/primary/everest"))
                .WillOnce(Return(SulDownloader::suldownloaderdata::IVersig::VerifySignature::SIGNATURE_VERIFIED));
        }
        else
        {
            EXPECT_CALL(*versig, verify(_, "/installroot/base/update/cache/primary/everest-plugin-a"))
                .WillOnce(Return(SulDownloader::suldownloaderdata::IVersig::VerifySignature::SIGNATURE_FAILED));
        }

        return std::unique_ptr<SulDownloader::suldownloaderdata::IVersig>(versig);
    });

    ProductReportVector productReports = defaultProductReports();
    productReports[0].productStatus = ProductReport::ProductStatus::VerifyFailed;
    productReports[1].productStatus = ProductReport::ProductStatus::VerifyFailed;

    // if up to distribution passed, warehouse never returns error = true
    EXPECT_CALL(mock, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(mock, synchronize(_));
    EXPECT_CALL(mock, distribute());
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/installroot/base/update/cache/primary/everest");
    products[1].setDistributePath("/installroot/base/update/cache/primary/everest-plugin-a");

    EXPECT_CALL(mock, getProducts()).WillOnce(Return(products));
    EXPECT_CALL(mock, getSourceURL());

    SimplifiedDownloadReport expectedDownloadReport{
        SulDownloader::suldownloaderdata::WarehouseStatus::INSTALLFAILED, "Update failed", productReports, false, {}
    };

    expectedDownloadReport.Products[1].errorDescription = "Product Everest-Plugins-A failed signature verification";
    DownloadReport previousDownloadReport = DownloadReport::Report("Not assigned");
    auto result = SulDownloader::runSULDownloader(configurationData, previousConfigurationData, previousDownloadReport);
    EXPECT_PRED_FORMAT2(downloadReportSimilar, expectedDownloadReport, result);
}

/**
 * Simulate error in installing base successfully but fail to install plugin
 */
TEST_F( // NOLINT
    SULDownloaderTest,
    runSULDownloader_PluginInstallationFailureShouldResultInValidInstalledFailedReport)
{
    auto& fileSystemMock = setupFileSystemAndGetMock();

    MockWarehouseRepository& mock = warehouseMocked();

    DownloadedProductVector products = defaultProducts();

    for (auto& product : products)
    {
        product.setProductHasChanged(true);
    }

    std::string everest_installer = "/installroot/base/update/cache/primary/everest/install.sh";
    std::string plugin_installer = "/installroot/base/update/cache/primary/everest-plugin-a/install.sh";
    EXPECT_CALL(fileSystemMock, exists(everest_installer)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory(everest_installer)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(everest_installer)).Times(1);
    EXPECT_CALL(fileSystemMock, exists(plugin_installer)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory(plugin_installer)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(plugin_installer)).Times(1);
    std::vector<std::string> emptyFileList;
    std::string uninstallPath = "/installroot/base/update/var/installedproducts";
    EXPECT_CALL(fileSystemMock, isDirectory(uninstallPath)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, listFiles(uninstallPath)).WillOnce(Return(emptyFileList));

    int counter = 0;

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&counter]() {
        if (counter++ == 0)
        {
            auto mockProcess = new StrictMock<MockProcess>();
            EXPECT_CALL(*mockProcess, exec(HasSubstr("everest/install.sh"), _, _)).Times(1);
            EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, output()).WillOnce(Return("installing base"));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        }
        else
        {
            auto mockProcess = new StrictMock<MockProcess>();
            EXPECT_CALL(*mockProcess, exec(HasSubstr("everest-plugin-a/install.sh"), _, _)).Times(1);
            EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, output()).WillOnce(Return("installing plugin\nsimulate failure"));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(5));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        }
    });

    ProductReportVector productReports = defaultProductReports();
    productReports[1].errorDescription = "Product Everest-Plugins-A failed to install";

    // base upgraded, plugin failed.
    productReports[0].productStatus = ProductReport::ProductStatus::Upgraded;
    productReports[1].productStatus = ProductReport::ProductStatus::InstallFailed;

    // if up to distribution passed, warehouse never returns error = true
    EXPECT_CALL(mock, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(mock, synchronize(_));
    EXPECT_CALL(mock, distribute());
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/installroot/base/update/cache/primary/everest");
    products[1].setDistributePath("/installroot/base/update/cache/primary/everest-plugin-a");
    EXPECT_CALL(mock, getProducts()).WillOnce(Return(products));
    EXPECT_CALL(mock, getSourceURL());
    EXPECT_CALL(mock, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
EXPECT_CALL(mock, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));    

    SimplifiedDownloadReport expectedDownloadReport{ SulDownloader::suldownloaderdata::WarehouseStatus::INSTALLFAILED,
                                                     "Update failed",
                                                     productReports,
                                                     false,
                                                     productsInfo({ products[0], products[1] }) };

    ConfigurationData configurationData = configData(defaultSettings());
    ConfigurationData previousConfigurationData;
    configurationData.verifySettingsAreValid();
    DownloadReport previousDownloadReport = DownloadReport::Report("Not assigned");

    EXPECT_PRED_FORMAT2(
        downloadReportSimilar,
        expectedDownloadReport,
        SulDownloader::runSULDownloader(configurationData, previousConfigurationData, previousDownloadReport));
}

/**
 * Simulate successful full update
 *
 */
TEST_F( // NOLINT
    SULDownloaderTest,
    runSULDownloader_SuccessfulFullUpdateShouldResultInValidSuccessReport)
{
    auto& fileSystemMock = setupFileSystemAndGetMock();
    MockWarehouseRepository& mock = warehouseMocked();

    DownloadedProductVector products = defaultProducts();

    for (auto& product : products)
    {
        product.setProductHasChanged(true);
    }

    std::string everest_installer = "/installroot/base/update/cache/primary/everest/install.sh";
    std::string plugin_installer = "/installroot/base/update/cache/primary/everest-plugin-a/install.sh";
    EXPECT_CALL(fileSystemMock, exists(everest_installer)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory(everest_installer)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(everest_installer)).Times(1);
    EXPECT_CALL(fileSystemMock, exists(plugin_installer)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory(plugin_installer)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(plugin_installer)).Times(1);
    std::vector<std::string> emptyFileList;
    std::string uninstallPath = "/installroot/base/update/var/installedproducts";
    EXPECT_CALL(fileSystemMock, isDirectory(uninstallPath)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, listFiles(uninstallPath)).WillOnce(Return(emptyFileList));

    int counter = 0;

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&counter]() {
        if (counter++ == 0)
        {
            auto mockProcess = new StrictMock<MockProcess>();
            EXPECT_CALL(*mockProcess, exec(HasSubstr("everest/install.sh"), _, _)).Times(1);
            EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, output()).WillOnce(Return("installing base"));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        }
        else
        {
            auto mockProcess = new StrictMock<MockProcess>();
            EXPECT_CALL(*mockProcess, exec(HasSubstr("everest-plugin-a/install.sh"), _, _)).Times(1);
            EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, output()).WillOnce(Return("installing plugin"));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        }
    });

    ProductReportVector productReports = defaultProductReports();
    productReports[0].productStatus = ProductReport::ProductStatus::Upgraded;
    productReports[1].productStatus = ProductReport::ProductStatus::Upgraded;

    // if up to distribution passed, warehouse never returns error = true
    EXPECT_CALL(mock, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(mock, synchronize(_));
    EXPECT_CALL(mock, distribute());
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/installroot/base/update/cache/primary/everest");
    products[1].setDistributePath("/installroot/base/update/cache/primary/everest-plugin-a");
    EXPECT_CALL(mock, getProducts()).WillOnce(Return(products));
    EXPECT_CALL(mock, getSourceURL());
    EXPECT_CALL(mock, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(mock, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));    
    SimplifiedDownloadReport expectedDownloadReport{ SulDownloader::suldownloaderdata::WarehouseStatus::SUCCESS,
                                                     "",
                                                     productReports,
                                                     true,
                                                     productsInfo({ products[0], products[1] }) };

    ConfigurationData configurationData = configData(defaultSettings());
    ConfigurationData previousConfigurationData;
    configurationData.verifySettingsAreValid();
    DownloadReport previousDownloadReport = DownloadReport::Report("Not assigned");

    auto calculatedReport = SulDownloader::runSULDownloader(configurationData, previousConfigurationData, previousDownloadReport); 
    EXPECT_PRED_FORMAT2(
        downloadReportSimilar,
        expectedDownloadReport,
        calculatedReport);
    auto subscriptions = calculatedReport.getSubscriptionComponents(); 
    std::vector<suldownloaderdata::SubscriptionInfo> expected = { 
        { products[0].getLine(), products[0].getProductMetadata().getVersion()},
        { products[1].getLine(), products[1].getProductMetadata().getVersion()}  };
    EXPECT_EQ( subscriptions, expected );        
}

TEST_F( // NOLINT
    SULDownloaderTest,
    runSULDownloader_SuccessfulFullUpdateWithSubscriptionsDifferentFromProductsShouldBeReportedCorrectly)
{
    auto& fileSystemMock = setupFileSystemAndGetMock();
    MockWarehouseRepository& mock = warehouseMocked();

    DownloadedProductVector products = defaultProducts();

    for (auto& product : products)
    {
        product.setProductHasChanged(true);
    }

    std::string everest_installer = "/installroot/base/update/cache/primary/everest/install.sh";
    std::string plugin_installer = "/installroot/base/update/cache/primary/everest-plugin-a/install.sh";
    EXPECT_CALL(fileSystemMock, exists(everest_installer)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory(everest_installer)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(everest_installer)).Times(1);
    EXPECT_CALL(fileSystemMock, exists(plugin_installer)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory(plugin_installer)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(plugin_installer)).Times(1);
    std::vector<std::string> emptyFileList;
    std::string uninstallPath = "/installroot/base/update/var/installedproducts";
    EXPECT_CALL(fileSystemMock, isDirectory(uninstallPath)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, listFiles(uninstallPath)).WillOnce(Return(emptyFileList));

    int counter = 0;

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&counter]() {
        if (counter++ == 0)
        {
            auto mockProcess = new StrictMock<MockProcess>();
            EXPECT_CALL(*mockProcess, exec(HasSubstr("everest/install.sh"), _, _)).Times(1);
            EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, output()).WillOnce(Return("installing base"));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        }
        else
        {
            auto mockProcess = new StrictMock<MockProcess>();
            EXPECT_CALL(*mockProcess, exec(HasSubstr("everest-plugin-a/install.sh"), _, _)).Times(1);
            EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, output()).WillOnce(Return("installing plugin"));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        }
    });

    ProductReportVector productReports = defaultProductReports();
    productReports[0].productStatus = ProductReport::ProductStatus::Upgraded;
    productReports[1].productStatus = ProductReport::ProductStatus::Upgraded;

    // if up to distribution passed, warehouse never returns error = true
    EXPECT_CALL(mock, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(mock, synchronize(_));
    EXPECT_CALL(mock, distribute());
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/installroot/base/update/cache/primary/everest");
    products[1].setDistributePath("/installroot/base/update/cache/primary/everest-plugin-a");
    EXPECT_CALL(mock, getProducts()).WillOnce(Return(products));
    EXPECT_CALL(mock, getSourceURL());
    EXPECT_CALL(mock, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(mock, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0] }))); // only product 0 in the subscriptions
    SimplifiedDownloadReport expectedDownloadReport{ SulDownloader::suldownloaderdata::WarehouseStatus::SUCCESS,
                                                     "",
                                                     productReports,
                                                     true,
                                                     productsInfo({ products[0], products[1] }) };                                                     

    ConfigurationData configurationData = configData(defaultSettings());
    ConfigurationData previousConfigurationData;
    configurationData.verifySettingsAreValid();
    DownloadReport previousDownloadReport = DownloadReport::Report("Not assigned");

    auto calculatedReport = SulDownloader::runSULDownloader(configurationData, previousConfigurationData, previousDownloadReport); 
    EXPECT_PRED_FORMAT2(
        downloadReportSimilar,
        expectedDownloadReport,
        calculatedReport);
   // only one component were returned in the subscription see listInstalledSubscriptions
   auto subscriptions = calculatedReport.getSubscriptionComponents(); 
    std::vector<suldownloaderdata::SubscriptionInfo> expected = { 
        { products[0].getLine(), products[0].getProductMetadata().getVersion()}};
    EXPECT_EQ( subscriptions, expected );

}

TEST_F( // NOLINT
        SULDownloaderTest,
        runSULDownloader_ListOfSubscriptionListSizeDifferenceResultsInFullSuccessfulUpdate)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(2);
    MockWarehouseRepository& mock = warehouseMocked();

    DownloadedProductVector products = defaultProducts();

    for (auto& product : products)
    {
        product.setProductHasChanged(false);
    }

    std::string everest_installer = "/installroot/base/update/cache/primary/everest/install.sh";
    std::string plugin_installer = "/installroot/base/update/cache/primary/everest-plugin-a/install.sh";
    EXPECT_CALL(fileSystemMock, exists(everest_installer)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory(everest_installer)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(everest_installer)).Times(1);
    EXPECT_CALL(fileSystemMock, exists(plugin_installer)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory(plugin_installer)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(plugin_installer)).Times(1);
    std::vector<std::string> emptyFileList;
    std::string uninstallPath = "/installroot/base/update/var/installedproducts";
    EXPECT_CALL(fileSystemMock, isDirectory(uninstallPath)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, listFiles(uninstallPath)).WillOnce(Return(emptyFileList));

    int counter = 0;

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&counter]() {
        if (counter++ == 0)
        {
            auto mockProcess = new StrictMock<MockProcess>();
            EXPECT_CALL(*mockProcess, exec(HasSubstr("everest/install.sh"), _, _)).Times(1);
            EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, output()).WillOnce(Return("installing base"));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        }
        else
        {
            auto mockProcess = new StrictMock<MockProcess>();
            EXPECT_CALL(*mockProcess, exec(HasSubstr("everest-plugin-a/install.sh"), _, _)).Times(1);
            EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, output()).WillOnce(Return("installing plugin"));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        }
    });

    ProductReportVector productReports = defaultProductReports();
    productReports[0].productStatus = ProductReport::ProductStatus::Upgraded;
    productReports[1].productStatus = ProductReport::ProductStatus::Upgraded;

    // if up to distribution passed, warehouse never returns error = true
    EXPECT_CALL(mock, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(mock, synchronize(_));
    EXPECT_CALL(mock, distribute());
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/installroot/base/update/cache/primary/everest");
    products[1].setDistributePath("/installroot/base/update/cache/primary/everest-plugin-a");
    EXPECT_CALL(mock, getProducts()).WillOnce(Return(products));
    EXPECT_CALL(mock, getSourceURL());
    EXPECT_CALL(mock, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(mock, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));    
    SimplifiedDownloadReport expectedDownloadReport{ SulDownloader::suldownloaderdata::WarehouseStatus::SUCCESS,
                                                     "",
                                                     productReports,
                                                     true,
                                                     productsInfo({ products[0], products[1] }) };

    ConfigurationData previousConfigurationData = configData(defaultSettings());
    ConfigurationData configurationData = configData(newSubscriptionSettings());

    configurationData.verifySettingsAreValid();
    previousConfigurationData.verifySettingsAreValid();

    TimeTracker timeTracker;
    timeTracker.setStartTime(std::time_t(0));
    timeTracker.setFinishedTime(std::time_t(0));

    DownloadReport previousDownloadReport =
            DownloadReport::Report("", products, {}, {}, &timeTracker, DownloadReport::VerifyState::VerifyCorrect);

    EXPECT_PRED_FORMAT2(
            downloadReportSimilar,
            expectedDownloadReport,
            SulDownloader::runSULDownloader(configurationData, previousConfigurationData, previousDownloadReport));

}

TEST_F( // NOLINT
        SULDownloaderTest,
        runSULDownloader_ListOfFeatureListSizeDifferenceResultsInFullSuccessfulUpdate)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(2);
    MockWarehouseRepository& mock = warehouseMocked();

    DownloadedProductVector products = defaultProducts();

    for (auto& product : products)
    {
        product.setProductHasChanged(false);
    }

    std::string everest_installer = "/installroot/base/update/cache/primary/everest/install.sh";
    std::string plugin_installer = "/installroot/base/update/cache/primary/everest-plugin-a/install.sh";
    EXPECT_CALL(fileSystemMock, exists(everest_installer)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory(everest_installer)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(everest_installer)).Times(1);
    EXPECT_CALL(fileSystemMock, exists(plugin_installer)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory(plugin_installer)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(plugin_installer)).Times(1);
    std::vector<std::string> emptyFileList;
    std::string uninstallPath = "/installroot/base/update/var/installedproducts";
    EXPECT_CALL(fileSystemMock, isDirectory(uninstallPath)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, listFiles(uninstallPath)).WillOnce(Return(emptyFileList));

    int counter = 0;

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&counter]() {
        if (counter++ == 0)
        {
            auto mockProcess = new StrictMock<MockProcess>();
            EXPECT_CALL(*mockProcess, exec(HasSubstr("everest/install.sh"), _, _)).Times(1);
            EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, output()).WillOnce(Return("installing base"));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        }
        else
        {
            auto mockProcess = new StrictMock<MockProcess>();
            EXPECT_CALL(*mockProcess, exec(HasSubstr("everest-plugin-a/install.sh"), _, _)).Times(1);
            EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, output()).WillOnce(Return("installing plugin"));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        }
    });

    ProductReportVector productReports = defaultProductReports();
    productReports[0].productStatus = ProductReport::ProductStatus::Upgraded;
    productReports[1].productStatus = ProductReport::ProductStatus::Upgraded;

    // if up to distribution passed, warehouse never returns error = true
    EXPECT_CALL(mock, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(mock, synchronize(_));
    EXPECT_CALL(mock, distribute());
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/installroot/base/update/cache/primary/everest");
    products[1].setDistributePath("/installroot/base/update/cache/primary/everest-plugin-a");
    EXPECT_CALL(mock, getProducts()).WillOnce(Return(products));
    EXPECT_CALL(mock, getSourceURL());
    EXPECT_CALL(mock, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(mock, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));    
    SimplifiedDownloadReport expectedDownloadReport{ SulDownloader::suldownloaderdata::WarehouseStatus::SUCCESS,
                                                     "",
                                                     productReports,
                                                     true,
                                                     productsInfo({ products[0], products[1] }) };

    ConfigurationData previousConfigurationData = configData(defaultSettings());
    ConfigurationData configurationData = configData(newFeatureSettings());

    configurationData.verifySettingsAreValid();
    previousConfigurationData.verifySettingsAreValid();

    TimeTracker timeTracker;
    timeTracker.setStartTime(std::time_t(0));
    timeTracker.setFinishedTime(std::time_t(0));

    DownloadReport previousDownloadReport =
            DownloadReport::Report("", products, {}, {}, &timeTracker, DownloadReport::VerifyState::VerifyCorrect);

    EXPECT_PRED_FORMAT2(
            downloadReportSimilar,
            expectedDownloadReport,
            SulDownloader::runSULDownloader(configurationData, previousConfigurationData, previousDownloadReport));

}

TEST_F( // NOLINT
        SULDownloaderTest,
        runSULDownloader_ListOfSubscriptionsWhichDontMatchPreviousConfigResultsInFullSuccessfulUpdate)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(2);
    MockWarehouseRepository& mock = warehouseMocked();

    DownloadedProductVector products = defaultProducts();

    for (auto& product : products)
    {
        product.setProductHasChanged(false);
    }

    std::string everest_installer = "/installroot/base/update/cache/primary/everest/install.sh";
    std::string plugin_installer = "/installroot/base/update/cache/primary/everest-plugin-a/install.sh";
    EXPECT_CALL(fileSystemMock, exists(everest_installer)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory(everest_installer)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(everest_installer)).Times(1);
    EXPECT_CALL(fileSystemMock, exists(plugin_installer)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory(plugin_installer)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(plugin_installer)).Times(1);
    std::vector<std::string> emptyFileList;
    std::string uninstallPath = "/installroot/base/update/var/installedproducts";
    EXPECT_CALL(fileSystemMock, isDirectory(uninstallPath)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, listFiles(uninstallPath)).WillOnce(Return(emptyFileList));

    int counter = 0;

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&counter]() {
        if (counter++ == 0)
        {
            auto mockProcess = new StrictMock<MockProcess>();
            EXPECT_CALL(*mockProcess, exec(HasSubstr("everest/install.sh"), _, _)).Times(1);
            EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, output()).WillOnce(Return("installing base"));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        }
        else
        {
            auto mockProcess = new StrictMock<MockProcess>();
            EXPECT_CALL(*mockProcess, exec(HasSubstr("everest-plugin-a/install.sh"), _, _)).Times(1);
            EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, output()).WillOnce(Return("installing plugin"));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        }
    });

    ProductReportVector productReports = defaultProductReports();
    productReports[0].productStatus = ProductReport::ProductStatus::Upgraded;
    productReports[1].productStatus = ProductReport::ProductStatus::Upgraded;

    // if up to distribution passed, warehouse never returns error = true
    EXPECT_CALL(mock, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(mock, synchronize(_));
    EXPECT_CALL(mock, distribute());
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/installroot/base/update/cache/primary/everest");
    products[1].setDistributePath("/installroot/base/update/cache/primary/everest-plugin-a");
    EXPECT_CALL(mock, getProducts()).WillOnce(Return(products));
    EXPECT_CALL(mock, getSourceURL());
    EXPECT_CALL(mock, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(mock, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));    
    SimplifiedDownloadReport expectedDownloadReport{ SulDownloader::suldownloaderdata::WarehouseStatus::SUCCESS,
                                                     "",
                                                     productReports,
                                                     true,
                                                     productsInfo({ products[0], products[1] }) };

    ConfigurationData previousConfigurationData = configData(newSubscriptionSettings());
    ConfigurationData configurationData = configData(newDifferentSubscriptionSettings());

    configurationData.verifySettingsAreValid();
    previousConfigurationData.verifySettingsAreValid();

    TimeTracker timeTracker;
    timeTracker.setStartTime(std::time_t(0));
    timeTracker.setFinishedTime(std::time_t(0));

    DownloadReport previousDownloadReport =
            DownloadReport::Report("", products, {}, {}, &timeTracker, DownloadReport::VerifyState::VerifyCorrect);

    EXPECT_PRED_FORMAT2(
            downloadReportSimilar,
            expectedDownloadReport,
            SulDownloader::runSULDownloader(configurationData, previousConfigurationData, previousDownloadReport));

}

TEST_F( // NOLINT
        SULDownloaderTest,
        runSULDownloader_ListOfFeaturesWhichDontMatchPreviousConfigResultsInFullSuccessfulUpdate)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(2);
    MockWarehouseRepository& mock = warehouseMocked();

    DownloadedProductVector products = defaultProducts();

    for (auto& product : products)
    {
        product.setProductHasChanged(false);
    }

    std::string everest_installer = "/installroot/base/update/cache/primary/everest/install.sh";
    std::string plugin_installer = "/installroot/base/update/cache/primary/everest-plugin-a/install.sh";
    EXPECT_CALL(fileSystemMock, exists(everest_installer)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory(everest_installer)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(everest_installer)).Times(1);
    EXPECT_CALL(fileSystemMock, exists(plugin_installer)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory(plugin_installer)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(plugin_installer)).Times(1);
    std::vector<std::string> emptyFileList;
    std::string uninstallPath = "/installroot/base/update/var/installedproducts";
    EXPECT_CALL(fileSystemMock, isDirectory(uninstallPath)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, listFiles(uninstallPath)).WillOnce(Return(emptyFileList));

    int counter = 0;

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&counter]() {
        if (counter++ == 0)
        {
            auto mockProcess = new StrictMock<MockProcess>();
            EXPECT_CALL(*mockProcess, exec(HasSubstr("everest/install.sh"), _, _)).Times(1);
            EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, output()).WillOnce(Return("installing base"));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        }
        else
        {
            auto mockProcess = new StrictMock<MockProcess>();
            EXPECT_CALL(*mockProcess, exec(HasSubstr("everest-plugin-a/install.sh"), _, _)).Times(1);
            EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, output()).WillOnce(Return("installing plugin"));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        }
    });

    ProductReportVector productReports = defaultProductReports();
    productReports[0].productStatus = ProductReport::ProductStatus::Upgraded;
    productReports[1].productStatus = ProductReport::ProductStatus::Upgraded;

    // if up to distribution passed, warehouse never returns error = true
    EXPECT_CALL(mock, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(mock, synchronize(_));
    EXPECT_CALL(mock, distribute());
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/installroot/base/update/cache/primary/everest");
    products[1].setDistributePath("/installroot/base/update/cache/primary/everest-plugin-a");
    EXPECT_CALL(mock, getProducts()).WillOnce(Return(products));
    EXPECT_CALL(mock, getSourceURL());
    EXPECT_CALL(mock, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(mock, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));    
    SimplifiedDownloadReport expectedDownloadReport{ SulDownloader::suldownloaderdata::WarehouseStatus::SUCCESS,
                                                     "",
                                                     productReports,
                                                     true,
                                                     productsInfo({ products[0], products[1] }) };

    ConfigurationData previousConfigurationData = configData(newFeatureSettings());
    ConfigurationData configurationData = configData(newDifferentFeatureSettings());

    configurationData.verifySettingsAreValid();
    previousConfigurationData.verifySettingsAreValid();


    TimeTracker timeTracker;
    timeTracker.setStartTime(std::time_t(0));
    timeTracker.setFinishedTime(std::time_t(0));

    DownloadReport previousDownloadReport =
            DownloadReport::Report("", products, {}, {}, &timeTracker, DownloadReport::VerifyState::VerifyCorrect);

    EXPECT_PRED_FORMAT2(
            downloadReportSimilar,
            expectedDownloadReport,
            SulDownloader::runSULDownloader(configurationData, previousConfigurationData, previousDownloadReport));
}

TEST_F( // NOLINT
        SULDownloaderTest,
        runSULDownloader_previousProductChangesShouldResultInSuccessfulFullUpdateAndProduceValidSuccessReport)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(1);
    MockWarehouseRepository& mock = warehouseMocked();

    DownloadedProductVector products = defaultProducts();

    for (auto& product : products)
    {
        product.setProductHasChanged(false);
    }

    std::string everest_installer = "/installroot/base/update/cache/primary/everest/install.sh";
    std::string plugin_installer = "/installroot/base/update/cache/primary/everest-plugin-a/install.sh";
    EXPECT_CALL(fileSystemMock, exists(everest_installer)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory(everest_installer)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(everest_installer)).Times(1);
    EXPECT_CALL(fileSystemMock, exists(plugin_installer)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory(plugin_installer)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(plugin_installer)).Times(1);
    std::vector<std::string> emptyFileList;
    std::string uninstallPath = "/installroot/base/update/var/installedproducts";
    EXPECT_CALL(fileSystemMock, isDirectory(uninstallPath)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, listFiles(uninstallPath)).WillOnce(Return(emptyFileList));

    int counter = 0;

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&counter]() {
        if (counter++ == 0)
        {
            auto mockProcess = new StrictMock<MockProcess>();
            EXPECT_CALL(*mockProcess, exec(HasSubstr("everest/install.sh"), _, _)).Times(1);
            EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, output()).WillOnce(Return("installing base"));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        }
        else
        {
            auto mockProcess = new StrictMock<MockProcess>();
            EXPECT_CALL(*mockProcess, exec(HasSubstr("everest-plugin-a/install.sh"), _, _)).Times(1);
            EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, output()).WillOnce(Return("installing plugin"));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        }
    });

    ProductReportVector productReports = defaultProductReports();
    productReports[0].productStatus = ProductReport::ProductStatus::Upgraded;
    productReports[1].productStatus = ProductReport::ProductStatus::Upgraded;

    // if up to distribution passed, warehouse never returns error = true
    EXPECT_CALL(mock, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(mock, synchronize(_));
    EXPECT_CALL(mock, distribute());
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/installroot/base/update/cache/primary/everest");
    products[1].setDistributePath("/installroot/base/update/cache/primary/everest-plugin-a");
    EXPECT_CALL(mock, getProducts()).WillOnce(Return(products));
    EXPECT_CALL(mock, getSourceURL());
    EXPECT_CALL(mock, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(mock, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));    
    SimplifiedDownloadReport expectedDownloadReport{ SulDownloader::suldownloaderdata::WarehouseStatus::SUCCESS,
                                                     "",
                                                     productReports,
                                                     true,
                                                     productsInfo({ products[0], products[1] }) };

    ConfigurationData configurationData = configData(defaultSettings());
    //ConfigurationData previousConfigurationData = configData(previousFeatureSettings());
    ConfigurationData previousConfigurationData;
    configurationData.verifySettingsAreValid();
    previousConfigurationData.verifySettingsAreValid();
    DownloadReport previousDownloadReport = DownloadReport::Report("Not assigned");

    EXPECT_PRED_FORMAT2(
            downloadReportSimilar,
            expectedDownloadReport,
            SulDownloader::runSULDownloader(configurationData, previousConfigurationData, previousDownloadReport));
}

TEST_F( // NOLINT
        SULDownloaderTest,
        runSULDownloader_WithUpdateConfigDataMatchingWarehouseSynchronizationResultingInNoUpdateNeededShouldCreateValidSuccessReport)
{
    auto& fileSystemMock = setupFileSystemAndGetMock();
    MockWarehouseRepository& mock = warehouseMocked();
    DownloadedProductVector products = defaultProducts();
    ProductReportVector productReports = defaultProductReports();

    for (auto& product : products)
    {
        product.setProductHasChanged(false);
    }

    EXPECT_CALL(mock, hasError()).WillRepeatedly(Return(false));

    EXPECT_CALL(mock, synchronize(_));
    EXPECT_CALL(mock, distribute());
    EXPECT_CALL(mock, getProducts()).WillOnce(Return(products));
    EXPECT_CALL(mock, getSourceURL());
    EXPECT_CALL(mock, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(mock, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));

    std::vector<std::string> emptyFileList;
    std::string uninstallPath = "/installroot/base/update/var/installedproducts";
    EXPECT_CALL(fileSystemMock, isDirectory(uninstallPath)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, listFiles(uninstallPath)).WillOnce(Return(emptyFileList));

    SimplifiedDownloadReport expectedDownloadReport{ SulDownloader::suldownloaderdata::WarehouseStatus::SUCCESS,
                                                     "",
                                                     productReports,
                                                     true,
                                                     productsInfo({ products[0], products[1] }) };

    ConfigurationData configurationData = configData(defaultSettings());
    ConfigurationData previousConfigurationData = configData(defaultSettings());
    configurationData.verifySettingsAreValid();

    TimeTracker timeTracker;
    timeTracker.setStartTime(std::time_t(0));
    timeTracker.setFinishedTime(std::time_t(0));
    DownloadReport previousDownloadReport =
            DownloadReport::Report("", products, {}, {}, &timeTracker, DownloadReport::VerifyState::VerifyCorrect);

    EXPECT_PRED_FORMAT2(
            downloadReportSimilar,
            expectedDownloadReport,
            SulDownloader::runSULDownloader(configurationData, previousConfigurationData, previousDownloadReport));
}

TEST_F( // NOLINT
    SULDownloaderTest,
    runSULDownloader_checkLogVerbosityVERBOSE)
{
    auto& fileSystem = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystem, isFile(_)).WillOnce(Return(false));
    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();
    auto settings = defaultSettings();
    settings.clear_sophosurls();
    settings.clear_cacheupdatesslpath();
    settings.add_sophosurls("http://localhost/latest/donotexits");
    settings.set_loglevel(ConfigurationSettings::VERBOSE);
    suldownloaderdata::ConfigurationData configurationData = configData(settings);
    suldownloaderdata::ConfigurationData previousConfigurationData;
    configurationData.verifySettingsAreValid();
    DownloadReport previousDownloadReport = DownloadReport::Report("Not assigned");
    auto downloadReport = SulDownloader::runSULDownloader(configurationData, previousConfigurationData, previousDownloadReport);
    std::string output = testing::internal::GetCapturedStdout();
    std::string errStd = testing::internal::GetCapturedStderr();
    ASSERT_THAT(errStd, ::testing::HasSubstr("Failed to connect to the warehouse"));
    ASSERT_THAT(errStd, ::testing::HasSubstr("Proxy used was"));
}

TEST_F( // NOLINT
    SULDownloaderTest,
    runSULDownloader_checkLogVerbosityNORMAL)
{
    auto& fileSystem = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystem, isFile(_)).WillOnce(Return(false));
    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();
    auto settings = defaultSettings();
    settings.clear_sophosurls();
    settings.clear_cacheupdatesslpath();
    settings.add_sophosurls("http://localhost/latest/donotexits");
    settings.set_loglevel(ConfigurationSettings::NORMAL);
    ConfigurationData configurationData = configData(settings);
    ConfigurationData previousConfigurationData;
    configurationData.verifySettingsAreValid();
    DownloadReport previousDownloadReport = DownloadReport::Report("Not assigned");
    auto downloadReport = SulDownloader::runSULDownloader(configurationData, previousConfigurationData, previousDownloadReport);
    std::string output = testing::internal::GetCapturedStdout();
    std::string errStd = testing::internal::GetCapturedStderr();
    ASSERT_THAT(output, ::testing::Not(::testing::HasSubstr("Proxy used was")));
    ASSERT_THAT(errStd, ::testing::Not(::testing::HasSubstr("Proxy used was")));
    ASSERT_THAT(errStd, ::testing::HasSubstr("Failed to connect to the warehouse"));
}
