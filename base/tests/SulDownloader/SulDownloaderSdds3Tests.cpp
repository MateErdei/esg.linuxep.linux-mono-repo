//Copyright 2018-2023 Sophos Limited.  All rights reserved.

/**
 * Component tests to SULDownloader mocking out Sdds3Repository
 */

#include "ConfigurationSettings.pb.h"
#include "MockSdds3Repository.h"
#include "MockVersig.h"
#include "TestSdds3RepositoryHelper.h"

#include "tests/Common/Helpers/LogInitializedTests.h"

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
#include <tests/Common/Helpers/MockProcess.h>

using namespace SulDownloader::suldownloaderdata;

using SulDownloaderProto::ConfigurationSettings;
struct Sdds3SimplifiedDownloadReport
{
    SulDownloader::suldownloaderdata::RepositoryStatus Status;
    std::string Description;
    std::vector<ProductReport> Products;
    bool shouldContainSyncTime;
    std::vector<ProductInfo> WarehouseComponents;
};

namespace
{
    using DownloadedProductVector = std::vector<SulDownloader::suldownloaderdata::DownloadedProduct>;
    using ProductReportVector = std::vector<SulDownloader::suldownloaderdata::ProductReport>;

    void setupInstalledFeaturesPermissions()
    {
        auto mockFilePermissions = new StrictMock<MockFilePermissions>();
        EXPECT_CALL(*mockFilePermissions, chown(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath(), sophos::updateSchedulerUser(), sophos::group()));
        EXPECT_CALL(*mockFilePermissions, chmod(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath(), S_IRUSR | S_IWUSR | S_IRGRP));
        auto mockIFilePermissionsPtr = std::unique_ptr<MockFilePermissions>(mockFilePermissions);
        Tests::replaceFilePermissions(std::move(mockIFilePermissionsPtr));
    }

} // namespace

class SULDownloaderSdds3Test : public ::testing::Test
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
          auto* versig = new NiceMock<MockVersig>();
          ON_CALL(*versig, verify(_, _))
              .WillByDefault(Return(SulDownloader::suldownloaderdata::IVersig::VerifySignature::SIGNATURE_VERIFIED));
          return std::unique_ptr<SulDownloader::suldownloaderdata::IVersig>(versig);
        });
        m_mockptr = nullptr;

        auto* mockPidLockFileUtilsPtr = new NiceMock<MockPidLockFileUtils>();
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
        Common::FileSystemImpl::restorePidLockUtils();
        SulDownloader::suldownloaderdata::VersigFactory::instance().restoreCreator();
        TestSdds3RepositoryHelper::restoreSdds3RepositoryFactory();
        Tests::restoreFilePermissions();
        Common::ApplicationConfiguration::restoreApplicationPathManager();
        Test::TearDown();
    }

    static std::vector<std::string> defaultOverrideSettings()
    {
        std::vector<std::string> lines = {
            "URLS = http://127.0.0.1:8080" ,
            "USE_SDDS3 = true",
            "USE_HTTP = true"};
        return lines;
    }
    static ConfigurationSettings defaultSettings()
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
        auto* proto_subscription = settings.mutable_primarysubscription();
        proto_subscription->set_rigidname("ServerProtectionLinux-Base-component");
        proto_subscription->set_baseversion("");
        proto_subscription->set_tag("RECOMMMENDED");
        proto_subscription->set_fixedversion("");
        settings.add_features("CORE");
        settings.mutable_jwtoken()->assign("token");
        settings.mutable_deviceid()->assign("deviceid");
        settings.mutable_tenantid()->assign("tenantid");
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
        auto* proto_subscriptions = settings.mutable_products();

        SulDownloaderProto::ConfigurationSettings_Subscription proto_subscription;

        proto_subscription.set_rigidname("Plugin_1");
        proto_subscription.set_baseversion("");
        proto_subscription.set_tag("RECOMMMENDED");
        proto_subscription.set_fixedversion("");

        proto_subscriptions->Add(
            dynamic_cast<SulDownloaderProto::ConfigurationSettings_Subscription&&>(proto_subscription));

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
            dynamic_cast<SulDownloaderProto::ConfigurationSettings_Subscription&&>(proto_subscription));

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
            product.setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary");
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
        base.setDefaultHomePath("ServerProtectionLinux-Base-component");
        base.setLine("ServerProtectionLinux-Base-component");
        base.setName("ServerProtectionLinux-Base-component-Product");
        base.setVersion("10.2.3");
        base.setTags({ { "RECOMMENDED", "10", "Base-label" } });

        SulDownloader::suldownloaderdata::ProductMetadata plugin;
        plugin.setDefaultHomePath("ServerProtectionLinux-Plugin-EDR");
        plugin.setLine("ServerProtectionLinux-Plugin-EDR");
        plugin.setName("ServerProtectionLinux-Plugin-EDR-Product");
        plugin.setVersion("10.3.5");
        plugin.setTags({ { "RECOMMENDED", "10", "Plugin-label" } });

        return std::vector<SulDownloader::suldownloaderdata::ProductMetadata>{ base, plugin };
    }

    ProductReportVector defaultProductReports()
    {
        SulDownloader::suldownloaderdata::ProductReport base;
        base.name = "ServerProtectionLinux-Base-component-Product";
        base.rigidName = "ServerProtectionLinux-Base-component";
        base.downloadedVersion = "10.2.3";
        base.productStatus = ProductReport::ProductStatus::UpToDate;
        SulDownloader::suldownloaderdata::ProductReport plugin;
        plugin.name = "ServerProtectionLinux-Plugin-EDR-Product";
        plugin.rigidName = "ServerProtectionLinux-Plugin-EDR";
        plugin.downloadedVersion = "10.3.5";
        plugin.productStatus = ProductReport::ProductStatus::UpToDate;
        return ProductReportVector{ base, plugin };
    }

    MockSdds3Repository& repositoryMocked(int resetCount = 1)
    {
        if (m_mockptr == nullptr)
        {
            m_mockptr = new StrictMock<MockSdds3Repository>();
            TestSdds3RepositoryHelper::replaceSdds3RepositoryCreator([this]() {
              return suldownloaderdata::ISDDS3RepositoryPtr(this->m_mockptr);
            });
        }
        return *m_mockptr;
    }

    MockFileSystem& setupFileSystemAndGetMock(int expectCallCount = 1, int expectCurrentProxy = 2, int expectedInstalledFeatures = 1, std::string installedFeatures = R"(["CORE"])")
    {
        auto* filesystemMock = new StrictMock<MockFileSystem>();
        EXPECT_CALL(*filesystemMock, isDirectory("/opt/sophos-spl")).Times(expectCallCount).WillRepeatedly(Return(true));
        EXPECT_CALL(*filesystemMock, isDirectory("/opt/sophos-spl/base/update/cache/"))
            .Times(expectCallCount)
            .WillRepeatedly(Return(true));

        EXPECT_CALL(*filesystemMock, exists(_)).WillRepeatedly(Return(true));
        EXPECT_CALL(*filesystemMock, currentWorkingDirectory()).WillRepeatedly(Return("/opt/sophos-spl/base/bin"));
        EXPECT_CALL(*filesystemMock, isFile("/opt/sophos-spl/base/etc/savedproxy.config")).WillRepeatedly(Return(false));
        std::string currentProxyFilePath =
            Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFilePath();
        EXPECT_CALL(*filesystemMock, isFile(currentProxyFilePath)).Times(expectCurrentProxy).WillOnce(Return(false));
        std::string installedFeaturesFile = Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath();
        EXPECT_CALL(*filesystemMock, writeFile(installedFeaturesFile, installedFeatures)).Times(expectedInstalledFeatures);

        setupExpectanceWriteProductUpdate(*filesystemMock);

        auto* pointer = filesystemMock;
        m_replacer.replace(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));
        return *pointer;
    }

    void setupFileVersionCalls(MockFileSystem& fileSystemMock, const std::string& currentVersion, const std::string& newVersion)
    {
        setupBaseVersionFileCalls(fileSystemMock, currentVersion, newVersion);
        setupPluginVersionFileCalls(fileSystemMock, currentVersion, newVersion);
    }

    void setupBaseVersionFileCalls(MockFileSystem& fileSystemMock, const std::string& currentVersion, const std::string& newVersion)
    {
        std::vector<std::string> currentVersionContents{{ currentVersion }} ;
        std::vector<std::string> newVersionContents{{newVersion}} ;

        EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component/VERSION.ini"))
            .WillOnce(Return(true));
        EXPECT_CALL(fileSystemMock, readLines("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component/VERSION.ini"))
            .WillOnce(Return(newVersionContents));
        EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/VERSION.ini"))
            .WillRepeatedly(Return(true));
        EXPECT_CALL(fileSystemMock, readLines("/opt/sophos-spl/base/VERSION.ini"))
            .WillRepeatedly(Return(currentVersionContents));

    }

    void setupPluginVersionFileCalls(MockFileSystem& fileSystemMock, const std::string& currentVersion, const std::string& newVersion)
    {
        std::vector<std::string> currentVersionContents{{ currentVersion }} ;
        std::vector<std::string> newVersionContents{{newVersion}} ;

        EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR/VERSION.ini"))
            .WillOnce(Return(true));
        EXPECT_CALL(fileSystemMock, readLines("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR/VERSION.ini"))
            .WillOnce(Return(newVersionContents));
        EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/installedproductversions/ServerProtectionLinux-Plugin-EDR.ini"))
            .WillRepeatedly(Return(true));
        EXPECT_CALL(fileSystemMock, readLines("/opt/sophos-spl/base/update/var/installedproductversions/ServerProtectionLinux-Plugin-EDR.ini"))
            .WillRepeatedly(Return(currentVersionContents));
    }

    void setupExpectanceWriteProductUpdate(MockFileSystem& mockFileSystem)
    {
        EXPECT_CALL(
            mockFileSystem, writeFile("/opt/sophos-spl/var/suldownloader_last_product_update.marker", "")
        ).Times(::testing::AtMost(1));
        EXPECT_CALL(
            mockFileSystem, writeFile(Common::ApplicationConfiguration::applicationPathManager().getUpdateMarkerFile(), "/opt/sophos-spl/base/bin")
        ).Times(::testing::AtMost(1));
        EXPECT_CALL(
            mockFileSystem, removeFile(Common::ApplicationConfiguration::applicationPathManager().getUpdateMarkerFile())
        ).Times(::testing::AtMost(1));
        EXPECT_CALL(
            mockFileSystem, isFile(Common::ApplicationConfiguration::applicationPathManager().getUpdateMarkerFile())
        ).Times(::testing::AtMost(1));
    }

    void setupExpectanceWriteAtomically(MockFileSystem& mockFileSystem, const std::string& contains, int installedFeaturesWritesExpected = 1)
    {
        EXPECT_CALL(
            mockFileSystem, writeFile(::testing::HasSubstr("/opt/sophos-spl/tmp"), ::testing::HasSubstr(contains)));
        EXPECT_CALL(mockFileSystem, moveFile(_, "/dir/output.json"));
        auto mockFilePermissions = new StrictMock<MockFilePermissions>();
        EXPECT_CALL(*mockFilePermissions, chown(testing::HasSubstr("/opt/sophos-spl/tmp"), sophos::updateSchedulerUser(), "root"));
        mode_t expectedFilePermissions = S_IRUSR | S_IWUSR;
        EXPECT_CALL(*mockFilePermissions, chmod(testing::HasSubstr("/opt/sophos-spl/tmp"), expectedFilePermissions));

        EXPECT_CALL(*mockFilePermissions, chown(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath(), sophos::updateSchedulerUser(), sophos::group())).Times(installedFeaturesWritesExpected);
        EXPECT_CALL(*mockFilePermissions, chmod(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath(), S_IRUSR | S_IWUSR | S_IRGRP)).Times(installedFeaturesWritesExpected);

        std::unique_ptr<MockFilePermissions> mockIFilePermissionsPtr =
            std::unique_ptr<MockFilePermissions>(mockFilePermissions);
        Tests::replaceFilePermissions(std::move(mockIFilePermissionsPtr));

    }

    ::testing::AssertionResult downloadReportSimilar(
        const char* m_expr,
        const char* n_expr,
        const Sdds3SimplifiedDownloadReport& expected,
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
            m_expr, n_expr, expected.WarehouseComponents, resulted.getRepositoryComponents());
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
    MockSdds3Repository* m_mockptr = nullptr;
    Tests::ScopedReplaceFileSystem m_replacer;

};

TEST_F(SULDownloaderSdds3Test, configurationDataVerificationOfDefaultSettingsReturnsTrue) // NOLINT
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock, isDirectory("/opt/sophos-spl")).Times(1).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, isDirectory("/opt/sophos-spl/base/update/cache/"))
        .Times(1)
        .WillRepeatedly(Return(true));

    EXPECT_CALL(*filesystemMock, exists(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, currentWorkingDirectory()).WillRepeatedly(Return("/opt/sophos-spl/base/bin"));
    EXPECT_CALL(*filesystemMock, isFile("/opt/sophos-spl/base/etc/savedproxy.config")).WillRepeatedly(Return(false));
    setupExpectanceWriteProductUpdate(*filesystemMock);

    m_replacer.replace(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));
    SulDownloader::suldownloaderdata::ConfigurationData confData = configData(defaultSettings());
    confData.verifySettingsAreValid();
    EXPECT_TRUE(confData.isVerified());
}

TEST_F(SULDownloaderSdds3Test, main_entry_InvalidArgumentsReturnsTheCorrectErrorCode) // NOLINT
{
    char** argsNotUsed = nullptr;
    EXPECT_EQ(SulDownloader::main_entry(2, argsNotUsed), -1);
    EXPECT_EQ(SulDownloader::main_entry(1, argsNotUsed), -1);
}

TEST_F(SULDownloaderSdds3Test, main_entry_missingInputFileCausesReturnsCorrectErrorCode) // NOLINT
{
    auto filesystemMock = new StrictMock<MockFileSystem>();
    m_replacer.replace(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));
    char* inputFileDoesNotExist[] = { const_cast<char*>("SulDownloader"),
                                      const_cast<char*>("inputfiledoesnotexists.json"),
                                      const_cast<char*>("/some/dir/createoutputpath.json") };

    EXPECT_CALL(*filesystemMock, isFile("supplement_only.marker")).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, isDirectory("/some/dir")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, isDirectory("/some/dir/createoutputpath.json")).WillOnce(Return(false));
    std::vector<Path> noOldReports;
    EXPECT_CALL(*filesystemMock, listFiles("/some/dir")).WillOnce(Return(noOldReports));
    EXPECT_CALL(*filesystemMock, readFile("inputfiledoesnotexists.json")).WillRepeatedly(Throw(IFileSystemException("Cannot read file")));

    EXPECT_EQ(SulDownloader::main_entry(3, inputFileDoesNotExist), -2);
}

TEST_F(SULDownloaderSdds3Test, main_entry_inputFileGivenAsDirReturnsCorrectErrorCode) // NOLINT
{
    auto filesystemMock = new MockFileSystem();
    m_replacer.replace(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    EXPECT_CALL(*filesystemMock, isDirectory("/opt/sophos-spl/directorypath")).WillOnce(Return(true));

    // directory can not be replaced by file
    Common::ProcessImpl::ArgcAndEnv args("SulDownloader", { "input.json", "/opt/sophos-spl/directorypath" }, {});

    EXPECT_EQ(SulDownloader::main_entry(3, args.argc()), -2);
}

TEST_F( // NOLINT
    SULDownloaderSdds3Test,
    main_entry_onSuccessWhileForcingUpdateAsPreviousDownloadReportDoesNotExistCreatesReportContainingExpectedSuccessResult)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(1, 2, 1);
    MockSdds3Repository& mock = repositoryMocked();

    auto products = defaultProducts();
    products[0].setProductHasChanged(false);
    products[1].setProductHasChanged(false);

    EXPECT_CALL(mock, tryConnect(_, _, _)).WillOnce(Return(true));
    EXPECT_CALL(mock, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primary")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primary")).WillOnce(Return(true));
    EXPECT_CALL(mock, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(mock, distribute());
    EXPECT_CALL(mock, purge());
    EXPECT_CALL(mock, setWillInstall(_)).Times(2);
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    EXPECT_CALL(mock, getProducts()).Times(2).WillRepeatedly(Return(products));
    EXPECT_CALL(mock, getSourceURL());
    EXPECT_CALL(mock, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(mock, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));

    std::vector<std::string> emptyFileList;
    EXPECT_CALL(fileSystemMock, readFile("/dir/input.json")).WillOnce(Return(jsonSettings(defaultSettings())));
    EXPECT_CALL(fileSystemMock, isFile("/dir/previous_update_config.json")).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, isFile("/dir/supplement_only.marker")).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/etc/savedproxy.config")).Times(2).WillRepeatedly(Return(false));
    EXPECT_CALL(fileSystemMock, isDirectory("/dir/output.json")).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, isDirectory("/dir")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, listFiles("/dir")).WillOnce(Return(emptyFileList));
    EXPECT_CALL(fileSystemMock, exists("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, readLines("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(defaultOverrideSettings()));
    setupFileVersionCalls(fileSystemMock, "PRODUCT_VERSION = 1.1.3.703", "PRODUCT_VERSION = 1.1.3.703");

    setupExpectanceWriteAtomically(
        fileSystemMock,
        SulDownloader::suldownloaderdata::toString(SulDownloader::suldownloaderdata::RepositoryStatus::SUCCESS), 1);
    std::string baseInstallPath = "/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component/install.sh";
    EXPECT_CALL(fileSystemMock, isDirectory(baseInstallPath)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(baseInstallPath));

    std::string pluginInstallPath = "/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR/install.sh";
    EXPECT_CALL(fileSystemMock, isDirectory(pluginInstallPath)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(pluginInstallPath));

    std::string uninstallPath = "/opt/sophos-spl/base/update/var/installedproducts";
    EXPECT_CALL(fileSystemMock, isDirectory(uninstallPath)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, listFiles(uninstallPath)).WillOnce(Return(emptyFileList));

    Common::ProcessImpl::ArgcAndEnv args("SulDownloader", { "/dir/input.json", "/dir/output.json" }, {});

    int counter = 0;
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&counter]() {
      if (counter++ == 0)
      {
          auto* mockProcess = new StrictMock<MockProcess>();
          EXPECT_CALL(*mockProcess, exec(HasSubstr("ServerProtectionLinux-Base-component/install.sh"), _, _)).Times(1);
          EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
          EXPECT_CALL(*mockProcess, output()).WillOnce(Return("installing base"));
          EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
          return std::unique_ptr<Common::Process::IProcess>(mockProcess);
      }
      else
      {
          auto* mockProcess = new StrictMock<MockProcess>();
          EXPECT_CALL(*mockProcess, exec(HasSubstr("ServerProtectionLinux-Plugin-EDR/install.sh"), _, _)).Times(1);
          EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
          EXPECT_CALL(*mockProcess, output()).WillOnce(Return("installing plugin"));
          EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
          return std::unique_ptr<Common::Process::IProcess>(mockProcess);
      }
    });

    EXPECT_EQ(SulDownloader::main_entry(3, args.argc()), 0);
}

TEST_F(SULDownloaderSdds3Test, main_entry_onSuccessCreatesReportContainingExpectedSuccessResult) // NOLINT
{
    auto& fileSystemMock = setupFileSystemAndGetMock();
    MockSdds3Repository& mock = repositoryMocked();

    auto products = defaultProducts();
    products[0].setProductHasChanged(false);
    products[1].setProductHasChanged(false);

    EXPECT_CALL(mock, tryConnect(_, _, _)).WillOnce(Return(true));
    EXPECT_CALL(mock, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primary")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primary")).WillOnce(Return(true));
    EXPECT_CALL(mock, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(mock, distribute());
    EXPECT_CALL(mock, purge());
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    EXPECT_CALL(mock, getProducts()).Times(2).WillRepeatedly(Return(products));
    EXPECT_CALL(mock, getSourceURL());
    EXPECT_CALL(mock, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(mock, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));

    setupFileVersionCalls(fileSystemMock, "PRODUCT_VERSION = 1.1.3.703", "PRODUCT_VERSION = 1.1.3.703");

    TimeTracker timeTracker;
    timeTracker.setStartTime(std::time_t(0));
    timeTracker.setFinishedTime(std::time_t(0));
    DownloadReport downloadReport =
        DownloadReport::Report("", products, {}, {}, &timeTracker, DownloadReport::VerifyState::VerifyCorrect);
    std::string previousJsonReport = DownloadReport::fromReport(downloadReport);
    std::string previousReportFilename = "update_report-previous.json";
    std::vector<std::string> previousReportFileList = { previousReportFilename };
    std::vector<std::string> emptyFileList;
    EXPECT_CALL(fileSystemMock, readFile("/dir/input.json")).WillOnce(Return(jsonSettings(defaultSettings())));
    EXPECT_CALL(fileSystemMock, isFile("/dir/previous_update_config.json")).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, isFile("/dir/supplement_only.marker")).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, isDirectory("/dir/output.json")).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, isDirectory("/dir")).WillOnce(Return(true));

    EXPECT_CALL(fileSystemMock, listFiles("/dir")).WillOnce(Return(previousReportFileList));
    EXPECT_CALL(fileSystemMock, readFile(previousReportFilename)).WillOnce(Return(previousJsonReport));
    EXPECT_CALL(fileSystemMock, exists("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, readLines("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(defaultOverrideSettings()));


    setupExpectanceWriteAtomically(
        fileSystemMock,
        SulDownloader::suldownloaderdata::toString(SulDownloader::suldownloaderdata::RepositoryStatus::SUCCESS));

    std::string uninstallPath = "/opt/sophos-spl/base/update/var/installedproducts";
    EXPECT_CALL(fileSystemMock, isDirectory(uninstallPath)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, listFiles(uninstallPath)).WillOnce(Return(emptyFileList));

    Common::ProcessImpl::ArgcAndEnv args("SulDownloader", { "/dir/input.json", "/dir/output.json" }, {});

    EXPECT_EQ(SulDownloader::main_entry(3, args.argc()), 0);
}

TEST_F( // NOLINT
    SULDownloaderSdds3Test,
    main_entry_onSuccessCreatesReportContainingExpectedSuccessResultEnsuringInvalidPreviousReportFilesAreIgnored)
{
    auto& fileSystemMock = setupFileSystemAndGetMock();
    MockSdds3Repository& mock = repositoryMocked();

    auto products = defaultProducts();
    products[0].setProductHasChanged(false);
    products[1].setProductHasChanged(false);

    EXPECT_CALL(mock, tryConnect(_, _, _)).WillOnce(Return(true));
    EXPECT_CALL(mock, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primary")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primary")).WillOnce(Return(true));

    EXPECT_CALL(mock, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(mock, distribute());
    EXPECT_CALL(mock, purge());
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    EXPECT_CALL(mock, getProducts()).Times(2).WillRepeatedly(Return(products));
    EXPECT_CALL(mock, getSourceURL());
    EXPECT_CALL(mock, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(mock, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));

    setupFileVersionCalls(fileSystemMock, "PRODUCT_VERSION = 1.1.3.703", "PRODUCT_VERSION = 1.1.3.703");

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
    EXPECT_CALL(fileSystemMock, readFile("/dir/input.json")).WillOnce(Return(jsonSettings(defaultSettings())));
    EXPECT_CALL(fileSystemMock, isFile("/dir/previous_update_config.json")).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, isFile("/dir/supplement_only.marker")).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, isDirectory("/dir/output.json")).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, isDirectory("/dir")).WillOnce(Return(true));

    EXPECT_CALL(fileSystemMock, listFiles("/dir")).WillOnce(Return(previousReportFileList));
    EXPECT_CALL(fileSystemMock, readFile(previousReportFilename)).WillOnce(Return(previousJsonReport));
    EXPECT_CALL(fileSystemMock, exists("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, readLines("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(defaultOverrideSettings()));



    setupExpectanceWriteAtomically(
        fileSystemMock,
        SulDownloader::suldownloaderdata::toString(SulDownloader::suldownloaderdata::RepositoryStatus::SUCCESS));

    std::string uninstallPath = "/opt/sophos-spl/base/update/var/installedproducts";
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
    SULDownloaderSdds3Test,
    main_entry_onSuccessCreatesReportContainingExpectedSuccessResultAndRemovesProduct)
{
    auto& fileSystemMock = setupFileSystemAndGetMock();
    MockSdds3Repository& mock = repositoryMocked();
    auto products = defaultProducts();
    products[0].setProductHasChanged(false);
    products[1].setProductHasChanged(false);

    EXPECT_CALL(mock, tryConnect(_, _, _)).WillOnce(Return(true));
    EXPECT_CALL(mock, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primary")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primary")).WillOnce(Return(true));
    EXPECT_CALL(mock, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(mock, distribute());
    EXPECT_CALL(mock, purge());
    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    EXPECT_CALL(mock, getProducts()).Times(2).WillRepeatedly(Return(products));
    EXPECT_CALL(mock, getSourceURL());
    EXPECT_CALL(mock, getProductDistributionPath(isProduct("productRemove1")))
        .WillOnce(Return("/opt/sophos-spl/base/update/cache/sdds3primary/productRemove1"));

    EXPECT_CALL(mock, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(mock, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));

    setupFileVersionCalls(fileSystemMock, "PRODUCT_VERSION = 1.1.3.703", "PRODUCT_VERSION = 1.1.3.703");

    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/installedproductversions/productRemove1.ini")).WillOnce(Return(false));

    TimeTracker timeTracker;
    timeTracker.setStartTime(std::time_t(0));
    timeTracker.setFinishedTime(std::time_t(0));
    DownloadReport downloadReport =
        DownloadReport::Report("", products, {}, {}, &timeTracker, DownloadReport::VerifyState::VerifyCorrect);
    std::string previousJsonReport = DownloadReport::fromReport(downloadReport);
    std::string previousReportFilename = "update_report-previous.json";
    std::vector<std::string> previousReportFileList = { previousReportFilename };
    std::vector<std::string> emptyFileList;

    EXPECT_CALL(fileSystemMock, readFile("/dir/input.json")).WillOnce(Return(jsonSettings(defaultSettings())));
    EXPECT_CALL(fileSystemMock, isFile("/dir/previous_update_config.json")).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, isFile("/dir/supplement_only.marker")).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, isDirectory("/dir/output.json")).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, isDirectory("/dir")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/sdds3primary/productRemove1")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, removeFileOrDirectory(_)).Times(1);

    EXPECT_CALL(fileSystemMock, listFiles("/dir")).WillOnce(Return(previousReportFileList));
    EXPECT_CALL(fileSystemMock, readFile(previousReportFilename)).WillOnce(Return(previousJsonReport));
    EXPECT_CALL(fileSystemMock, exists("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, readLines("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(defaultOverrideSettings()));


    setupExpectanceWriteAtomically(
        fileSystemMock,
        SulDownloader::suldownloaderdata::toString(SulDownloader::suldownloaderdata::RepositoryStatus::SUCCESS));

    std::vector<std::string> fileListOfProductsToRemove = { "productRemove1.sh" };
    std::string uninstallPath = "/opt/sophos-spl/base/update/var/installedproducts";
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
    SULDownloaderSdds3Test,
    main_entry_onSuccessCreatesReportContainingExpectedUninstallFailedResult)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(1, 2, 0);
    MockSdds3Repository& mock = repositoryMocked();

    auto products = defaultProducts();
    products[0].setProductHasChanged(false);
    products[1].setProductHasChanged(false);

    EXPECT_CALL(mock, tryConnect(_, _, _)).WillOnce(Return(true));
    EXPECT_CALL(mock, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primary")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primary")).WillOnce(Return(true));
    EXPECT_CALL(mock, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(mock, distribute());
    EXPECT_CALL(mock, purge());
    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    EXPECT_CALL(mock, getProducts()).Times(2).WillRepeatedly(Return(products));
    EXPECT_CALL(mock, getSourceURL());
    EXPECT_CALL(mock, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(mock, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));
    EXPECT_CALL(mock, getProductDistributionPath(isProduct("productRemove1"))).WillOnce(Return("productRemove1"));

    setupFileVersionCalls(fileSystemMock, "PRODUCT_VERSION = 1.1.3.703", "PRODUCT_VERSION = 1.1.3.703");

    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/installedproductversions/productRemove1.ini")).WillOnce(Return(false));

    TimeTracker timeTracker;
    timeTracker.setStartTime(std::time_t(0));
    timeTracker.setFinishedTime(std::time_t(0));
    DownloadReport downloadReport =
        DownloadReport::Report("", products, {}, {}, &timeTracker, DownloadReport::VerifyState::VerifyCorrect);
    std::string previousJsonReport = DownloadReport::fromReport(downloadReport);
    std::string previousReportFilename = "update_report-previous.json";
    std::vector<std::string> previousReportFileList = { previousReportFilename };
    std::vector<std::string> emptyFileList;

    EXPECT_CALL(fileSystemMock, readFile("/dir/input.json")).WillOnce(Return(jsonSettings(defaultSettings())));
    EXPECT_CALL(fileSystemMock, isFile("/dir/previous_update_config.json")).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, isFile("/dir/supplement_only.marker")).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, isDirectory("/dir/output.json")).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, isDirectory("/dir")).WillOnce(Return(true));

    EXPECT_CALL(fileSystemMock, listFiles("/dir")).WillOnce(Return(previousReportFileList));
    EXPECT_CALL(fileSystemMock, readFile(previousReportFilename)).WillOnce(Return(previousJsonReport));
    EXPECT_CALL(fileSystemMock, exists("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, readLines("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(defaultOverrideSettings()));


    setupExpectanceWriteAtomically(
        fileSystemMock,
        SulDownloader::suldownloaderdata::toString(SulDownloader::suldownloaderdata::RepositoryStatus::UNINSTALLFAILED), 0);

    std::vector<std::string> fileListOfProductsToRemove = { "productRemove1.sh" };
    std::string uninstallPath = "/opt/sophos-spl/base/update/var/installedproducts";
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
        SulDownloader::main_entry(3, args.argc()), SulDownloader::suldownloaderdata::RepositoryStatus::UNINSTALLFAILED);
}

// the other execution paths were covered in main_entry_* tests.
TEST_F( // NOLINT
    SULDownloaderSdds3Test,
    fileEntriesAndRunDownloaderThrowIfCannotCreateOutputFile)
{
    auto filesystemMock = new MockFileSystem();
    m_replacer.replace(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));
    EXPECT_CALL(*filesystemMock, readFile("/dir/input.json")).WillRepeatedly(Return(jsonSettings(defaultSettings())));
    EXPECT_CALL(*filesystemMock, isDirectory("/dir/path/that/cannot/be/created/output.json")).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, isDirectory("/dir/path/that/cannot/be/created")).WillOnce(Return(false));

    EXPECT_THROW( // NOLINT
        SulDownloader::fileEntriesAndRunDownloader("/dir/input.json", "/dir/path/that/cannot/be/created/output.json", ""),
        SulDownloader::suldownloaderdata::SulDownloaderException);
}

// configAndRunDownloader
TEST_F( // NOLINT
    SULDownloaderSdds3Test,
    configAndRunDownloaderInvalidSettingsReportError_RepositoryStatus_UNSPECIFIED)
{
    m_replacer.replace(std::unique_ptr<Common::FileSystem::IFileSystem>(new MockFileSystem()));
    std::string reportContent;
    int exitCode = 0;
    auto settings = defaultSettings();
    settings.clear_sophosurls(); // no sophos urls, the system can not connect to warehouses
    std::string settingsString = jsonSettings(settings);
    std::string previousSettingString;
    std::string previousReportData;
    bool baseDowngraded = false;
    std::tie(exitCode, reportContent, baseDowngraded) =
        SulDownloader::configAndRunDownloader("inputFile", previousSettingString, previousReportData);

    EXPECT_NE(exitCode, 0);
    EXPECT_THAT(
        reportContent,
        ::testing::Not(::testing::HasSubstr(
            SulDownloader::suldownloaderdata::toString(SulDownloader::suldownloaderdata::RepositoryStatus::SUCCESS))));
    EXPECT_THAT(
        reportContent,
        ::testing::HasSubstr(SulDownloader::suldownloaderdata::toString(
            SulDownloader::suldownloaderdata::RepositoryStatus::UNSPECIFIED)));
}

// runSULDownloader
TEST_F( // NOLINT
    SULDownloaderSdds3Test,
    runSULDownloader_RepositoryConnectionFailureShouldCreateValidConnectionFailureReport)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(1,1, 0);
    MockSdds3Repository& mock = repositoryMocked();
    SulDownloader::suldownloaderdata::RepositoryError wError;
    wError.Description = "Error description";
    wError.status = SulDownloader::suldownloaderdata::RepositoryStatus::CONNECTIONERROR;
    std::string statusError = SulDownloader::suldownloaderdata::toString(wError.status);
    DownloadedProductVector emptyProducts;

    EXPECT_CALL(mock, hasError()).WillOnce(Return(true)).WillRepeatedly(Return(true));
    EXPECT_CALL(mock, hasImmediateFailError()).WillRepeatedly(Return(true));
    EXPECT_CALL(mock, getError()).WillRepeatedly(Return(wError));
    EXPECT_CALL(mock, tryConnect(_, _, _)).WillRepeatedly(Return(false)); // failed tryConnect call
    EXPECT_CALL(mock, getProducts()).WillOnce(Return(emptyProducts));
    EXPECT_CALL(mock, listInstalledSubscriptions()).WillOnce(Return(subscriptionsFromProduct(emptyProducts)));

    EXPECT_CALL(mock, getSourceURL());
    EXPECT_CALL(fileSystemMock, exists("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, readLines("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(defaultOverrideSettings()));

    Sdds3SimplifiedDownloadReport expectedDownloadReport{ wError.status, wError.Description, {}, false, {} };

    ConfigurationData configurationData = configData(defaultSettings());
    ConfigurationData previousConfigurationData;
    configurationData.verifySettingsAreValid();
    DownloadReport previousDownloadReport = DownloadReport::Report("Not assigned");

    DownloadReport actualDownloadReport = SulDownloader::runSULDownloader(
        configurationData, previousConfigurationData, previousDownloadReport,
        false);

    ASSERT_FALSE(actualDownloadReport.isSupplementOnlyUpdate());

    EXPECT_PRED_FORMAT2(
        downloadReportSimilar,
        expectedDownloadReport,
        actualDownloadReport);
}

// runSULDownloader
TEST_F( // NOLINT
    SULDownloaderSdds3Test,
    runSULDownloader_RepositoryConnectionFailureShouldCreateValidConnectionFailureReportContainingPreviousReportDFata)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(1,1, 0);
    MockSdds3Repository& mock = repositoryMocked();
    SulDownloader::suldownloaderdata::RepositoryError wError;
    wError.Description = "Error description";
    wError.status = SulDownloader::suldownloaderdata::RepositoryStatus::CONNECTIONERROR;
    std::string statusError = SulDownloader::suldownloaderdata::toString(wError.status);
    DownloadedProductVector emptyProducts;
    DownloadedProductVector products = defaultProducts();
    ProductReportVector productReports = defaultProductReports();

    TimeTracker timeTracker;
    timeTracker.setStartTime(std::time_t(0));
    timeTracker.setFinishedTime(std::time_t(0));

    std::string previousReportAsString{
        R"sophos({ "startTime": "20190604 144145", "finishTime": "20190604 144155",
                "syncTime": "20190604 144155", "status": "SUCCESS", "sulError": "", "errorDescription": "",
                "urlSource": "https://localhost:1233",
                "products": [
                { "rigidName": "ServerProtectionLinux-Base-component", "productName": "ServerProtectionLinux-Base-component-Product", "downloadVersion": "10.2.3", "errorDescription": "", "productStatus": "UPTODATE" },
                { "rigidName": "ServerProtectionLinux-Plugin-EDR", "productName": "ServerProtectionLinux-Plugin-EDR-Product", "downloadVersion": "10.3.5", "errorDescription": "", "productStatus": "UPTODATE" } ] })sophos"
    };
    DownloadReport previousDownloadReport = DownloadReport::toReport(previousReportAsString);

    EXPECT_CALL(mock, hasError()).WillOnce(Return(true)).WillRepeatedly(Return(true));
    EXPECT_CALL(mock, hasImmediateFailError()).WillRepeatedly(Return(true));
    EXPECT_CALL(mock, getError()).WillRepeatedly(Return(wError));
    EXPECT_CALL(mock, tryConnect(_, _, _)).WillRepeatedly(Return(false)); // failed tryConnect call
    EXPECT_CALL(mock, getProducts()).WillOnce(Return(emptyProducts));
    EXPECT_CALL(mock, listInstalledSubscriptions()).WillOnce(Return(subscriptionsFromProduct(emptyProducts)));

    EXPECT_CALL(mock, getSourceURL());
    EXPECT_CALL(fileSystemMock, exists("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, readLines("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(defaultOverrideSettings()));

    Sdds3SimplifiedDownloadReport expectedDownloadReport{ wError.status, wError.Description, productReports, false, {} };

    ConfigurationData configurationData = configData(defaultSettings());
    ConfigurationData previousConfigurationData;
    configurationData.verifySettingsAreValid();

    DownloadReport actualDownloadReport = SulDownloader::runSULDownloader(
        configurationData, previousConfigurationData, previousDownloadReport,
        false);

    ASSERT_FALSE(actualDownloadReport.isSupplementOnlyUpdate());

    EXPECT_PRED_FORMAT2(
        downloadReportSimilar,
        expectedDownloadReport,
        actualDownloadReport);
}



// runSULDownloader
TEST_F( // NOLINT
    SULDownloaderSdds3Test,
    runSULDownloader_supplement_only_RepositoryConnectionFailureShouldCreateValidConnectionFailureReport)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(1,1, 0);
    MockSdds3Repository& mock = repositoryMocked(2);  // reset called another time
    SulDownloader::suldownloaderdata::RepositoryError wError;
    wError.Description = "Error description";
    wError.status = SulDownloader::suldownloaderdata::RepositoryStatus::CONNECTIONERROR;
    std::string statusError = SulDownloader::suldownloaderdata::toString(wError.status);
    DownloadedProductVector emptyProducts;

    EXPECT_CALL(mock, hasError()).WillOnce(Return(true)).WillRepeatedly(Return(true));
    EXPECT_CALL(mock, hasImmediateFailError()).WillRepeatedly(Return(false));
    EXPECT_CALL(mock, getError()).WillRepeatedly(Return(wError));
    EXPECT_CALL(mock, tryConnect(_, true, _)).WillRepeatedly(Return(false)); // failed tryConnect call
    EXPECT_CALL(mock, getProducts()).WillOnce(Return(emptyProducts));
    EXPECT_CALL(mock, listInstalledSubscriptions()).WillOnce(Return(subscriptionsFromProduct(emptyProducts)));

    EXPECT_CALL(mock, getSourceURL());
    EXPECT_CALL(fileSystemMock, exists("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, readLines("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(defaultOverrideSettings()));

    Sdds3SimplifiedDownloadReport expectedDownloadReport{ wError.status, wError.Description, {}, false, {} };

    ConfigurationData configurationData = configData(defaultSettings());
    ConfigurationData previousConfigurationData;
    configurationData.verifySettingsAreValid();
    DownloadReport previousDownloadReport = DownloadReport::Report("Not assigned");

    DownloadReport actualDownloadReport = SulDownloader::runSULDownloader(
        configurationData, previousConfigurationData, previousDownloadReport,
        true);

    ASSERT_FALSE(actualDownloadReport.isSupplementOnlyUpdate());

    EXPECT_PRED_FORMAT2(
        downloadReportSimilar,
        expectedDownloadReport,
        actualDownloadReport);
}

TEST_F( // NOLINT
    SULDownloaderSdds3Test,
    runSULDownloader_RepositorySynchronizationFailureShouldCreateValidSyncronizationFailureReport)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(1, 2, 0);
    MockSdds3Repository& mock = repositoryMocked();
    SulDownloader::suldownloaderdata::RepositoryError wError;
    wError.Description = "Error description";
    wError.status = SulDownloader::suldownloaderdata::RepositoryStatus::PACKAGESOURCEMISSING;
    std::string statusError = SulDownloader::suldownloaderdata::toString(wError.status);
    DownloadedProductVector emptyProducts;

    EXPECT_CALL(mock, hasError())
        .WillOnce(Return(false)) // connection
        .WillOnce(Return(true))  // synchronization
        .WillRepeatedly(Return(true));
    EXPECT_CALL(mock, tryConnect(_, _, _)).WillOnce(Return(true)); // successful tryConnect call
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primary")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primary")).WillOnce(Return(true));
    EXPECT_CALL(mock, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(mock, getError()).WillRepeatedly(Return(wError));
    EXPECT_CALL(mock, getProducts()).WillOnce(Return(emptyProducts));
    EXPECT_CALL(mock, listInstalledSubscriptions()).WillOnce(Return(subscriptionsFromProduct(emptyProducts)));

    EXPECT_CALL(mock, getSourceURL());
    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/installedproductversions/ServerProtectionLinux-Plugin-EDR.ini"))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, exists("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, readLines("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(defaultOverrideSettings()));


    Sdds3SimplifiedDownloadReport expectedDownloadReport{ wError.status, wError.Description, {}, false, {} };

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
TEST_F(SULDownloaderSdds3Test, runSULDownloader_onDistributeFailure) // NOLINT
{
    auto& fileSystemMock = setupFileSystemAndGetMock(1, 2, 0);
    MockSdds3Repository& mock = repositoryMocked();
    SulDownloader::suldownloaderdata::RepositoryError wError;
    wError.Description = "Error description";
    wError.status = SulDownloader::suldownloaderdata::RepositoryStatus::DOWNLOADFAILED;
    std::string statusError = SulDownloader::suldownloaderdata::toString(wError.status);
    DownloadedProductVector products = defaultProducts();

    SulDownloader::suldownloaderdata::RepositoryError productError;
    productError.Description = "Product Error description";
    productError.status = SulDownloader::suldownloaderdata::RepositoryStatus::DOWNLOADFAILED;

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
    EXPECT_CALL(mock, tryConnect(_, _, _)).WillOnce(Return(true)); // successful tryConnect call
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primary")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primary")).WillOnce(Return(true));
    EXPECT_CALL(mock, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(mock, distribute());
    EXPECT_CALL(mock, setWillInstall(_)).Times(2);

    EXPECT_CALL(mock, getError()).WillRepeatedly(Return(wError));
    EXPECT_CALL(mock, getProducts()).WillRepeatedly(Return(products));
    EXPECT_CALL(mock, listInstalledSubscriptions()).WillOnce(Return(subscriptionsFromProduct(products)));

    EXPECT_CALL(mock, getSourceURL());
    EXPECT_CALL(fileSystemMock, exists("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, readLines("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(defaultOverrideSettings()));


    Sdds3SimplifiedDownloadReport expectedDownloadReport{ wError.status, wError.Description, productReports, false, {} };

    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/VERSION.ini"))
        .WillRepeatedly(Return(false));
    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/installedproductversions/ServerProtectionLinux-Plugin-EDR.ini"))
        .WillRepeatedly(Return(false));

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
    SULDownloaderSdds3Test,
    runSULDownloader_RepositorySynchronizationResultingInNoUpdateNeededShouldCreateValidSuccessReport)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(1, 2, 0);
    MockSdds3Repository& mock = repositoryMocked();
    DownloadedProductVector products = defaultProducts();
    ProductReportVector productReports = defaultProductReports();

    for (auto& product : products)
    {
        product.setProductHasChanged(false);
    }

    EXPECT_CALL(mock, hasError()).WillRepeatedly(Return(false));

    EXPECT_CALL(mock, tryConnect(_, _, _)).WillOnce(Return(true)); // successful tryConnect call
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primary")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primary")).WillOnce(Return(true));
    EXPECT_CALL(mock, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(mock, distribute());
    EXPECT_CALL(mock, purge());
    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    EXPECT_CALL(mock, getProducts()).Times(2).WillRepeatedly(Return(products));
    EXPECT_CALL(mock, getSourceURL());
    EXPECT_CALL(mock, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(mock, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));
    std::vector<std::string> emptyFileList;
    std::string uninstallPath = "/opt/sophos-spl/base/update/var/installedproducts";
    EXPECT_CALL(fileSystemMock, isDirectory(uninstallPath)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, listFiles(uninstallPath)).WillOnce(Return(emptyFileList));
    EXPECT_CALL(fileSystemMock, exists("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, readLines("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(defaultOverrideSettings()));


    setupFileVersionCalls(fileSystemMock, "PRODUCT_VERSION = 1.1.3.703", "PRODUCT_VERSION = 1.1.3.703");

    Sdds3SimplifiedDownloadReport expectedDownloadReport{ SulDownloader::suldownloaderdata::RepositoryStatus::SUCCESS,
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
    SULDownloaderSdds3Test,
    runSULDownloader_UpdateFailForInvalidSignature)
{
    MockFileSystem& fileSystemMock = setupFileSystemAndGetMock(1, 2, 0);
    MockSdds3Repository& mock = repositoryMocked();
    ConfigurationData configurationData = configData(defaultSettings());
    ConfigurationData previousConfigurationData;
    configurationData.verifySettingsAreValid();

    DownloadedProductVector products = defaultProducts();

    for (auto& product : products)
    {
        product.setProductHasChanged(true);
    }
    std::string everest_installer = "/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component/install.sh";
    std::string plugin_installer = "/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR/install.sh";
    int counter = 0;

    setupFileVersionCalls(fileSystemMock, "PRODUCT_VERSION = 1.1.3.0", "PRODUCT_VERSION = 1.1.3.703");
    EXPECT_CALL(fileSystemMock, exists("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, readLines("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(defaultOverrideSettings()));


    SulDownloader::suldownloaderdata::VersigFactory::instance().replaceCreator([&counter]() {
      auto versig = new StrictMock<MockVersig>();
      if (counter++ == 0)
      {
          EXPECT_CALL(*versig, verify(_, "/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component"))
              .WillOnce(Return(SulDownloader::suldownloaderdata::IVersig::VerifySignature::SIGNATURE_VERIFIED));
      }
      else
      {
          EXPECT_CALL(*versig, verify(_, "/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR"))
              .WillOnce(Return(SulDownloader::suldownloaderdata::IVersig::VerifySignature::SIGNATURE_FAILED));
      }

      return std::unique_ptr<SulDownloader::suldownloaderdata::IVersig>(versig);
    });

    ProductReportVector productReports = defaultProductReports();
    productReports[0].productStatus = ProductReport::ProductStatus::VerifyFailed;
    productReports[1].productStatus = ProductReport::ProductStatus::VerifyFailed;

    // if up to distribution passed, warehouse never returns error = true
    EXPECT_CALL(mock, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(mock, tryConnect(_, _, _)).WillOnce(Return(true)); // successful tryConnect call
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primary")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primary")).WillOnce(Return(true));
    EXPECT_CALL(mock, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(mock, distribute());
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    EXPECT_CALL(mock, setWillInstall(_)).Times(2);
    EXPECT_CALL(mock, getProducts()).WillRepeatedly(Return(products));
    EXPECT_CALL(mock, listInstalledSubscriptions()).WillOnce(Return(subscriptionsFromProduct(products)));
    EXPECT_CALL(mock, getSourceURL());

    Sdds3SimplifiedDownloadReport expectedDownloadReport{
        SulDownloader::suldownloaderdata::RepositoryStatus::INSTALLFAILED, "Update failed", productReports, false, {}
    };

    expectedDownloadReport.Products[1].errorDescription = "Product ServerProtectionLinux-Plugin-EDR failed signature verification";
    DownloadReport previousDownloadReport = DownloadReport::Report("Not assigned");
    auto result = SulDownloader::runSULDownloader(configurationData, previousConfigurationData, previousDownloadReport);
    EXPECT_PRED_FORMAT2(downloadReportSimilar, expectedDownloadReport, result);
}

/**
 * Simulate error in installing base successfully but fail to install plugin
 */
TEST_F( // NOLINT
    SULDownloaderSdds3Test,
    runSULDownloader_PluginInstallationFailureShouldResultInValidInstalledFailedReport)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(1, 2, 0);
    MockSdds3Repository& mock = repositoryMocked();

    DownloadedProductVector products = defaultProducts();

    for (auto& product : products)
    {
        product.setProductHasChanged(true);
    }

    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    std::string everest_installer = "/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component/install.sh";
    std::string plugin_installer = "/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR/install.sh";
    EXPECT_CALL(fileSystemMock, exists(everest_installer)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory(everest_installer)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(everest_installer)).Times(1);
    EXPECT_CALL(fileSystemMock, exists(plugin_installer)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory(plugin_installer)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(plugin_installer)).Times(1);
    std::vector<std::string> emptyFileList;
    std::string uninstallPath = "/opt/sophos-spl/base/update/var/installedproducts";
    EXPECT_CALL(fileSystemMock, isDirectory(uninstallPath)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, listFiles(uninstallPath)).WillOnce(Return(emptyFileList));
    EXPECT_CALL(fileSystemMock, exists("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, readLines("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(defaultOverrideSettings()));


    setupFileVersionCalls(fileSystemMock, "PRODUCT_VERSION = 1.1.3.0", "PRODUCT_VERSION = 1.1.3.703");

    int counter = 0;

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&counter]() {
      if (counter++ == 0)
      {
          auto mockProcess = new StrictMock<MockProcess>();
          EXPECT_CALL(*mockProcess, exec(HasSubstr("ServerProtectionLinux-Base-component/install.sh"), _, _)).Times(1);
          EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
          EXPECT_CALL(*mockProcess, output()).WillOnce(Return("installing base"));
          EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
          return std::unique_ptr<Common::Process::IProcess>(mockProcess);
      }
      else
      {
          auto mockProcess = new StrictMock<MockProcess>();
          EXPECT_CALL(*mockProcess, exec(HasSubstr("ServerProtectionLinux-Plugin-EDR/install.sh"), _, _)).Times(1);
          EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
          EXPECT_CALL(*mockProcess, output()).WillOnce(Return("installing plugin\nsimulate failure"));
          EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(5));
          return std::unique_ptr<Common::Process::IProcess>(mockProcess);
      }
    });

    ProductReportVector productReports = defaultProductReports();
    productReports[1].errorDescription = "Product ServerProtectionLinux-Plugin-EDR failed to install";

    // base upgraded, plugin failed.
    productReports[0].productStatus = ProductReport::ProductStatus::Upgraded;
    productReports[1].productStatus = ProductReport::ProductStatus::InstallFailed;

    // if up to distribution passed, warehouse never returns error = true
    EXPECT_CALL(mock, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(mock, tryConnect(_, _, _)).WillOnce(Return(true)); // successful tryConnect call
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primary")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primary")).WillOnce(Return(true));
    EXPECT_CALL(mock, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(mock, distribute());
    EXPECT_CALL(mock, purge());
    EXPECT_CALL(mock, setWillInstall(_)).Times(2);
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    EXPECT_CALL(mock, getProducts()).WillRepeatedly(Return(products));
    EXPECT_CALL(mock, getSourceURL());
    EXPECT_CALL(mock, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(mock, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));

    Sdds3SimplifiedDownloadReport expectedDownloadReport{ SulDownloader::suldownloaderdata::RepositoryStatus::INSTALLFAILED,
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
    SULDownloaderSdds3Test,
    runSULDownloader_SuccessfulFullUpdateShouldResultInValidSuccessReport)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(1, 2, 0);
    MockSdds3Repository& mock = repositoryMocked();
    DownloadedProductVector products = defaultProducts();

    for (auto& product : products)
    {
        product.setProductHasChanged(true);
    }

    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    std::string everest_installer = "/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component/install.sh";
    std::string plugin_installer = "/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR/install.sh";
    EXPECT_CALL(fileSystemMock, exists(everest_installer)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory(everest_installer)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(everest_installer)).Times(1);
    EXPECT_CALL(fileSystemMock, exists(plugin_installer)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory(plugin_installer)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(plugin_installer)).Times(1);
    std::vector<std::string> emptyFileList;
    std::string uninstallPath = "/opt/sophos-spl/base/update/var/installedproducts";
    EXPECT_CALL(fileSystemMock, isDirectory(uninstallPath)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, listFiles(uninstallPath)).WillOnce(Return(emptyFileList));
    EXPECT_CALL(fileSystemMock, exists("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, readLines("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(defaultOverrideSettings()));


    setupFileVersionCalls(fileSystemMock, "PRODUCT_VERSION = 1.1.3.0", "PRODUCT_VERSION = 1.1.3.703");

    int counter = 0;

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&counter]() {
      if (counter++ == 0)
      {
          auto mockProcess = new StrictMock<MockProcess>();
          EXPECT_CALL(*mockProcess, exec(HasSubstr("ServerProtectionLinux-Base-component/install.sh"), _, _)).Times(1);
          EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
          EXPECT_CALL(*mockProcess, output()).WillOnce(Return("installing base"));
          EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
          return std::unique_ptr<Common::Process::IProcess>(mockProcess);
      }
      else
      {
          auto mockProcess = new StrictMock<MockProcess>();
          EXPECT_CALL(*mockProcess, exec(HasSubstr("ServerProtectionLinux-Plugin-EDR/install.sh"), _, _)).Times(1);
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
    EXPECT_CALL(mock, tryConnect(_, _, _)).WillOnce(Return(true)); // successful tryConnect call
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primary")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primary")).WillOnce(Return(true));
    EXPECT_CALL(mock, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(mock, distribute());
    EXPECT_CALL(mock, purge());
    EXPECT_CALL(mock, setWillInstall(_)).Times(2);
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    EXPECT_CALL(mock, getProducts()).Times(2).WillRepeatedly(Return(products));
    EXPECT_CALL(mock, getSourceURL());
    EXPECT_CALL(mock, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(mock, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));
    Sdds3SimplifiedDownloadReport expectedDownloadReport{ SulDownloader::suldownloaderdata::RepositoryStatus::SUCCESS,
                                                     "",
                                                     productReports,
                                                     true,
                                                     productsInfo({ products[0], products[1] }) };

    ConfigurationData configurationData = configData(defaultSettings());
    ConfigurationData previousConfigurationData;
    configurationData.verifySettingsAreValid();
    DownloadReport previousDownloadReport = DownloadReport::Report("Not assigned");

    auto calculatedReport =
        SulDownloader::runSULDownloader(configurationData, previousConfigurationData, previousDownloadReport);
    EXPECT_PRED_FORMAT2(downloadReportSimilar, expectedDownloadReport, calculatedReport);
    auto productsAndSubscriptions = calculatedReport.getProducts();
    ASSERT_EQ(productsAndSubscriptions.size(), 2);
    EXPECT_EQ(productsAndSubscriptions[0].rigidName, products[0].getLine());
    EXPECT_EQ(productsAndSubscriptions[1].rigidName, products[1].getLine());
}

TEST_F( // NOLINT
    SULDownloaderSdds3Test,
    runSULDownloader_SuccessfulUpdateToNewVersionShouldNotRunUninstallScriptsAndShouldResultInValidSuccessReport)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(1, 2, 0);
    MockSdds3Repository& mock = repositoryMocked();
    DownloadedProductVector products = defaultProducts();

    for (auto& product : products)
    {
        product.setProductHasChanged(true);
    }
    std::string everest_installer = "/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component/install.sh";
    std::string plugin_installer = "/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR/install.sh";
    EXPECT_CALL(fileSystemMock, exists(everest_installer)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory(everest_installer)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(everest_installer)).Times(1);
    EXPECT_CALL(fileSystemMock, exists(plugin_installer)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory(plugin_installer)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(plugin_installer)).Times(1);
    std::vector<std::string> emptyFileList;
    std::string uninstallPath = "/opt/sophos-spl/base/update/var/installedproducts";
    EXPECT_CALL(fileSystemMock, isDirectory(uninstallPath)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, listFiles(uninstallPath)).WillOnce(Return(emptyFileList));
    EXPECT_CALL(fileSystemMock, exists("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, readLines("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(defaultOverrideSettings()));


    setupFileVersionCalls(fileSystemMock, "PRODUCT_VERSION = 1.1.3.703", "PRODUCT_VERSION = 1.1.3.703");

    int counter = 0;

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&counter]() {
      if (counter++ == 0)
      {
          auto mockProcess = new StrictMock<MockProcess>();

          EXPECT_CALL(*mockProcess, exec(HasSubstr("ServerProtectionLinux-Base-component/install.sh"), _, _)).Times(1);
          EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
          EXPECT_CALL(*mockProcess, output()).WillOnce(Return("installing base"));
          EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
          EXPECT_CALL(*mockProcess, exec(HasSubstr("bin/uninstall.sh"), _, _)).Times(0);
          return std::unique_ptr<Common::Process::IProcess>(mockProcess);
      }
      else
      {
          auto mockProcess = new StrictMock<MockProcess>();
          EXPECT_CALL(*mockProcess, exec(HasSubstr("ServerProtectionLinux-Plugin-EDR/install.sh"), _, _)).Times(1);
          EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
          EXPECT_CALL(*mockProcess, output()).WillOnce(Return("installing plugin"));
          EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
          EXPECT_CALL(*mockProcess, exec(HasSubstr("installedproducts/ServerProtectionLinux-Plugin-EDR.sh"), _, _)).Times(0);
          return std::unique_ptr<Common::Process::IProcess>(mockProcess);
      }
    });

    ProductReportVector productReports = defaultProductReports();
    productReports[0].productStatus = ProductReport::ProductStatus::Upgraded;
    productReports[1].productStatus = ProductReport::ProductStatus::Upgraded;

    // if up to distribution passed, warehouse never returns error = true
    EXPECT_CALL(mock, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(mock, tryConnect(_, _, _)).WillOnce(Return(true)); // successful tryConnect call
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primary")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primary")).WillOnce(Return(true));
    EXPECT_CALL(mock, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(mock, distribute());
    EXPECT_CALL(mock, purge());
    EXPECT_CALL(mock, setWillInstall(_)).Times(2);
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    EXPECT_CALL(mock, getProducts()).Times(2).WillRepeatedly(Return(products));
    EXPECT_CALL(mock, getSourceURL());
    EXPECT_CALL(mock, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(mock, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));
    Sdds3SimplifiedDownloadReport expectedDownloadReport{ SulDownloader::suldownloaderdata::RepositoryStatus::SUCCESS,
                                                     "",
                                                     productReports,
                                                     true,
                                                     productsInfo({ products[0], products[1] }) };

    ConfigurationData configurationData = configData(defaultSettings());
    ConfigurationData previousConfigurationData;
    configurationData.verifySettingsAreValid();
    DownloadReport previousDownloadReport = DownloadReport::Report("Not assigned");

    auto calculatedReport =
        SulDownloader::runSULDownloader(configurationData, previousConfigurationData, previousDownloadReport);
    EXPECT_PRED_FORMAT2(downloadReportSimilar, expectedDownloadReport, calculatedReport);
    auto productsAndSubscriptions = calculatedReport.getProducts();
    ASSERT_EQ(productsAndSubscriptions.size(), 2);
    EXPECT_EQ(productsAndSubscriptions[0].rigidName, products[0].getLine());
    EXPECT_EQ(productsAndSubscriptions[1].rigidName, products[1].getLine());

}


TEST_F( // NOLINT
    SULDownloaderSdds3Test,
    runSULDownloader_SuccessfulUpdateToOlderVersionShouldRunUninstallScriptsAndShouldResultInValidSuccessReport)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(1, 2, 0);
    MockSdds3Repository& mock = repositoryMocked();
    DownloadedProductVector products = defaultProducts();

    for (auto& product : products)
    {
        product.setProductHasChanged(true);
    }
    std::string everest_installer = "/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component/install.sh";
    std::string plugin_installer = "/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR/install.sh";
    EXPECT_CALL(fileSystemMock, exists(everest_installer)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory(everest_installer)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(everest_installer)).Times(1);
    EXPECT_CALL(fileSystemMock, exists(plugin_installer)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory(plugin_installer)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(plugin_installer)).Times(1);
    std::vector<std::string> emptyFileList;
    std::string uninstallPath = "/opt/sophos-spl/base/update/var/installedproducts";
    EXPECT_CALL(fileSystemMock, isDirectory(uninstallPath)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/bin/uninstall.sh")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isExecutable("/opt/sophos-spl/bin/uninstall.sh")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, listFiles(uninstallPath)).WillOnce(Return(emptyFileList));
    EXPECT_CALL(fileSystemMock, exists("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, readLines("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(defaultOverrideSettings()));


    setupFileVersionCalls(fileSystemMock, "PRODUCT_VERSION = 1.1.3.703", "PRODUCT_VERSION = 1.1.3.0");

    int counter = 0;

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&counter]() {
      if (counter == 0)
      {
          counter++;
          auto mockProcess = new StrictMock<MockProcess>();

          EXPECT_CALL(*mockProcess, exec(HasSubstr("bin/uninstall.sh"), _, _)).Times(1);
          EXPECT_CALL(*mockProcess, output()).Times(1);
          EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
          return std::unique_ptr<Common::Process::IProcess>(mockProcess);
      }
      else if (counter == 1)
      {
          counter++;
          auto mockProcess = new StrictMock<MockProcess>();

          EXPECT_CALL(*mockProcess, exec(HasSubstr("ServerProtectionLinux-Base-component/install.sh"), _, _)).Times(1);
          EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
          EXPECT_CALL(*mockProcess, output()).WillOnce(Return("installing base"));
          EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
          return std::unique_ptr<Common::Process::IProcess>(mockProcess);
      }
      else if (counter == 2)
      {
          counter++;
          auto mockProcess = new StrictMock<MockProcess>();
          EXPECT_CALL(*mockProcess, exec(HasSubstr("ServerProtectionLinux-Plugin-EDR/install.sh"), _, _)).Times(1);
          EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
          EXPECT_CALL(*mockProcess, output()).WillOnce(Return("installing plugin"));
          EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
          return std::unique_ptr<Common::Process::IProcess>(mockProcess);
      }
      else
      {
          counter++;
          auto mockProcess = new StrictMock<MockProcess>();
          EXPECT_CALL(*mockProcess, exec(HasSubstr("installedproducts/ServerProtectionLinux-Plugin-EDR.sh"), _, _)).Times(0);
          return std::unique_ptr<Common::Process::IProcess>(mockProcess);
      }
    });

    ProductReportVector productReports = defaultProductReports();
    productReports[0].productStatus = ProductReport::ProductStatus::Upgraded;
    productReports[1].productStatus = ProductReport::ProductStatus::Upgraded;

    // if up to distribution passed, warehouse never returns error = true
    EXPECT_CALL(mock, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(mock, tryConnect(_, _, _)).WillOnce(Return(true)); // successful tryConnect call
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primary")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primary")).WillOnce(Return(true));
    EXPECT_CALL(mock, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(mock, distribute());
    EXPECT_CALL(mock, purge());
    EXPECT_CALL(mock, setWillInstall(_)).Times(2);
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    EXPECT_CALL(mock, getProducts()).Times(2).WillRepeatedly(Return(products));
    EXPECT_CALL(mock, getSourceURL());
    EXPECT_CALL(mock, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(mock, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));
    Sdds3SimplifiedDownloadReport expectedDownloadReport{ SulDownloader::suldownloaderdata::RepositoryStatus::SUCCESS,
                                                     "",
                                                     productReports,
                                                     true,
                                                     productsInfo({ products[0], products[1] }) };

    ConfigurationData configurationData = configData(defaultSettings());
    ConfigurationData previousConfigurationData;
    configurationData.verifySettingsAreValid();
    DownloadReport previousDownloadReport = DownloadReport::Report("Not assigned");

    auto calculatedReport =
        SulDownloader::runSULDownloader(configurationData, previousConfigurationData, previousDownloadReport);
    EXPECT_PRED_FORMAT2(downloadReportSimilar, expectedDownloadReport, calculatedReport);
    auto productsAndSubscriptions = calculatedReport.getProducts();
    ASSERT_EQ(productsAndSubscriptions.size(), 2);
    EXPECT_EQ(productsAndSubscriptions[0].rigidName, products[0].getLine());
    EXPECT_EQ(productsAndSubscriptions[1].rigidName, products[1].getLine());
}

TEST_F( // NOLINT
    SULDownloaderSdds3Test,
    runSULDownloader_SuccessfulUpdateToOlderVersionOfPluginShouldOnlyRunPluginUninstallScriptsAndShouldResultInValidSuccessReport)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(1, 2, 0);
    MockSdds3Repository& mock = repositoryMocked();
    DownloadedProductVector products = defaultProducts();

    for (auto& product : products)
    {
        product.setProductHasChanged(true);
    }
    std::string everest_installer = "/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component/install.sh";
    std::string plugin_installer = "/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR/install.sh";
    EXPECT_CALL(fileSystemMock, exists(everest_installer)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory(everest_installer)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(everest_installer)).Times(1);
    EXPECT_CALL(fileSystemMock, exists(plugin_installer)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory(plugin_installer)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(plugin_installer)).Times(1);
    std::vector<std::string> emptyFileList;
    std::string uninstallPath = "/opt/sophos-spl/base/update/var/installedproducts";
    EXPECT_CALL(fileSystemMock, isDirectory(uninstallPath)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/installedproducts/ServerProtectionLinux-Plugin-EDR.sh")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isExecutable("/opt/sophos-spl/base/update/var/installedproducts/ServerProtectionLinux-Plugin-EDR.sh")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, listFiles(uninstallPath)).WillOnce(Return(emptyFileList));
    EXPECT_CALL(fileSystemMock, exists("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, readLines("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(defaultOverrideSettings()));

    setupBaseVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 1.1.3.7", "PRODUCT_VERSION = 1.1.3.7");
    setupPluginVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 1.1.3.7", "PRODUCT_VERSION = 1.1.3.0");

    int counter = 0;

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&counter]() {
      if (counter == 0)
      {
          counter++;
          auto mockProcess = new StrictMock<MockProcess>();

          EXPECT_CALL(*mockProcess, exec(HasSubstr("ServerProtectionLinux-Plugin-EDR.sh"), _, _)).Times(1);
          EXPECT_CALL(*mockProcess, output()).Times(1);
          EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
          return std::unique_ptr<Common::Process::IProcess>(mockProcess);
      }
      else if (counter == 1)
      {
          counter++;
          auto mockProcess = new StrictMock<MockProcess>();

          EXPECT_CALL(*mockProcess, exec(HasSubstr("ServerProtectionLinux-Base-component/install.sh"), _, _)).Times(1);
          EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
          EXPECT_CALL(*mockProcess, output()).WillOnce(Return("installing base"));
          EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
          return std::unique_ptr<Common::Process::IProcess>(mockProcess);
      }
      else if (counter == 2)
      {
          counter++;
          auto mockProcess = new StrictMock<MockProcess>();
          EXPECT_CALL(*mockProcess, exec(HasSubstr("ServerProtectionLinux-Plugin-EDR/install.sh"), _, _)).Times(1);
          EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
          EXPECT_CALL(*mockProcess, output()).WillOnce(Return("installing plugin"));
          EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
          return std::unique_ptr<Common::Process::IProcess>(mockProcess);
      }
      else
      {
          counter++;
          auto mockProcess = new StrictMock<MockProcess>();
          EXPECT_CALL(*mockProcess, exec(HasSubstr("installedproducts/ServerProtectionLinux-Plugin-EDR.sh"), _, _)).Times(0);
          return std::unique_ptr<Common::Process::IProcess>(mockProcess);
      }
    });

    ProductReportVector productReports = defaultProductReports();
    productReports[0].productStatus = ProductReport::ProductStatus::Upgraded;
    productReports[1].productStatus = ProductReport::ProductStatus::Upgraded;

    // if up to distribution passed, warehouse never returns error = true
    EXPECT_CALL(mock, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(mock, tryConnect(_, _, _)).WillOnce(Return(true)); // successful tryConnect call
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primary")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primary")).WillOnce(Return(true));
    EXPECT_CALL(mock, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(mock, distribute());
    EXPECT_CALL(mock, purge());
    EXPECT_CALL(mock, setWillInstall(_)).Times(2);
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    EXPECT_CALL(mock, getProducts()).Times(2).WillRepeatedly(Return(products));
    EXPECT_CALL(mock, getSourceURL());
    EXPECT_CALL(mock, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(mock, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));
    Sdds3SimplifiedDownloadReport expectedDownloadReport{ SulDownloader::suldownloaderdata::RepositoryStatus::SUCCESS,
                                                     "",
                                                     productReports,
                                                     true,
                                                     productsInfo({ products[0], products[1] }) };

    ConfigurationData configurationData = configData(defaultSettings());
    ConfigurationData previousConfigurationData;
    configurationData.verifySettingsAreValid();
    DownloadReport previousDownloadReport = DownloadReport::Report("Not assigned");

    auto calculatedReport =
        SulDownloader::runSULDownloader(configurationData, previousConfigurationData, previousDownloadReport);
    EXPECT_PRED_FORMAT2(downloadReportSimilar, expectedDownloadReport, calculatedReport);
    auto productsAndSubscriptions = calculatedReport.getProducts();
    ASSERT_EQ(productsAndSubscriptions.size(), 2);
    EXPECT_EQ(productsAndSubscriptions[0].rigidName, products[0].getLine());
    EXPECT_EQ(productsAndSubscriptions[1].rigidName, products[1].getLine());
}

TEST_F( // NOLINT
    SULDownloaderSdds3Test,
    runSULDownloader_SuccessfulFullUpdateWithSubscriptionsDifferentFromProductsShouldBeReportedCorrectly)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(1, 2, 0);
    MockSdds3Repository& mock = repositoryMocked();
    DownloadedProductVector products = defaultProducts();

    for (auto& product : products)
    {
        product.setProductHasChanged(true);
    }

    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    std::string everest_installer = "/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component/install.sh";
    std::string plugin_installer = "/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR/install.sh";
    EXPECT_CALL(fileSystemMock, exists(everest_installer)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory(everest_installer)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(everest_installer)).Times(1);
    EXPECT_CALL(fileSystemMock, exists(plugin_installer)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory(plugin_installer)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(plugin_installer)).Times(1);
    std::vector<std::string> emptyFileList;
    std::string uninstallPath = "/opt/sophos-spl/base/update/var/installedproducts";
    EXPECT_CALL(fileSystemMock, isDirectory(uninstallPath)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, listFiles(uninstallPath)).WillOnce(Return(emptyFileList));
    EXPECT_CALL(fileSystemMock, exists("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, readLines("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(defaultOverrideSettings()));


    setupFileVersionCalls(fileSystemMock, "PRODUCT_VERSION = 1.1.3.703", "PRODUCT_VERSION = 1.1.3.703");

    int counter = 0;

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&counter]() {
      if (counter++ == 0)
      {
          auto mockProcess = new StrictMock<MockProcess>();
          EXPECT_CALL(*mockProcess, exec(HasSubstr("ServerProtectionLinux-Base-component/install.sh"), _, _)).Times(1);
          EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
          EXPECT_CALL(*mockProcess, output()).WillOnce(Return("installing base"));
          EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
          return std::unique_ptr<Common::Process::IProcess>(mockProcess);
      }
      else
      {
          auto mockProcess = new StrictMock<MockProcess>();
          EXPECT_CALL(*mockProcess, exec(HasSubstr("ServerProtectionLinux-Plugin-EDR/install.sh"), _, _)).Times(1);
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
    EXPECT_CALL(mock, tryConnect(_, _, _)).WillOnce(Return(true)); // successful tryConnect call
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primary")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primary")).WillOnce(Return(true));
    EXPECT_CALL(mock, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(mock, distribute());
    EXPECT_CALL(mock, purge());
    EXPECT_CALL(mock, setWillInstall(_)).Times(2);
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    EXPECT_CALL(mock, getProducts()).Times(2).WillRepeatedly(Return(products));
    EXPECT_CALL(mock, getSourceURL());
    EXPECT_CALL(mock, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(mock, listInstalledSubscriptions)
        .WillOnce(Return(subscriptionsInfo({ products[0] }))); // only product 0 in the subscriptions
    Sdds3SimplifiedDownloadReport expectedDownloadReport{ SulDownloader::suldownloaderdata::RepositoryStatus::SUCCESS,
                                                     "",
                                                     productReports,
                                                     true,
                                                     productsInfo({ products[0], products[1] }) };

    auto copyProduct = expectedDownloadReport.Products[0];
    expectedDownloadReport.Products.clear();
    expectedDownloadReport.Products.push_back(copyProduct);

    ConfigurationData configurationData = configData(defaultSettings());
    ConfigurationData previousConfigurationData;
    configurationData.verifySettingsAreValid();
    DownloadReport previousDownloadReport = DownloadReport::Report("Not assigned");

    auto calculatedReport =
        SulDownloader::runSULDownloader(configurationData, previousConfigurationData, previousDownloadReport);
    EXPECT_PRED_FORMAT2(downloadReportSimilar, expectedDownloadReport, calculatedReport);
    // only one component were returned in the subscription see listInstalledSubscriptions
    auto subscriptions = calculatedReport.getProducts();
    ASSERT_EQ(subscriptions.size(), 1);
    EXPECT_EQ(subscriptions[0].rigidName, products[0].getProductMetadata().getLine());
}

TEST_F( // NOLINT
    SULDownloaderSdds3Test,
    runSULDownloader_ListOfSubscriptionListSizeDifferenceResultsInFullSuccessfulUpdate)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(2, 2, 0);
    MockSdds3Repository& mock = repositoryMocked();
    DownloadedProductVector products = defaultProducts();

    for (auto& product : products)
    {
        product.setProductHasChanged(false);
    }

    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    std::string everest_installer = "/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component/install.sh";
    std::string plugin_installer = "/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR/install.sh";
    EXPECT_CALL(fileSystemMock, exists(everest_installer)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory(everest_installer)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(everest_installer)).Times(1);
    EXPECT_CALL(fileSystemMock, exists(plugin_installer)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory(plugin_installer)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(plugin_installer)).Times(1);
    std::vector<std::string> emptyFileList;
    std::string uninstallPath = "/opt/sophos-spl/base/update/var/installedproducts";
    EXPECT_CALL(fileSystemMock, isDirectory(uninstallPath)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, listFiles(uninstallPath)).WillOnce(Return(emptyFileList));
    EXPECT_CALL(fileSystemMock, exists("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, readLines("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(defaultOverrideSettings()));


    setupFileVersionCalls(fileSystemMock, "PRODUCT_VERSION = 1.1.3.703", "PRODUCT_VERSION = 1.1.3.703");

    int counter = 0;

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&counter]() {
      if (counter++ == 0)
      {
          auto mockProcess = new StrictMock<MockProcess>();
          EXPECT_CALL(*mockProcess, exec(HasSubstr("ServerProtectionLinux-Base-component/install.sh"), _, _)).Times(1);
          EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
          EXPECT_CALL(*mockProcess, output()).WillOnce(Return("installing base"));
          EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
          return std::unique_ptr<Common::Process::IProcess>(mockProcess);
      }
      else
      {
          auto mockProcess = new StrictMock<MockProcess>();
          EXPECT_CALL(*mockProcess, exec(HasSubstr("ServerProtectionLinux-Plugin-EDR/install.sh"), _, _)).Times(1);
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
    EXPECT_CALL(mock, tryConnect(_, _, _)).WillOnce(Return(true)); // successful tryConnect call
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primary")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primary")).WillOnce(Return(true));
    EXPECT_CALL(mock, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(mock, distribute());
    EXPECT_CALL(mock, purge());
    EXPECT_CALL(mock, setWillInstall(_)).Times(1);
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    EXPECT_CALL(mock, getProducts()).WillRepeatedly(Return(products));
    EXPECT_CALL(mock, getSourceURL());
    EXPECT_CALL(mock, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(mock, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));
    Sdds3SimplifiedDownloadReport expectedDownloadReport{ SulDownloader::suldownloaderdata::RepositoryStatus::SUCCESS,
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
    SULDownloaderSdds3Test,
    runSULDownloader_ListOfFeatureListSizeDifferenceResultsInFullSuccessfulUpdate)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(2, 2, 0);
    MockSdds3Repository& mock = repositoryMocked();
    DownloadedProductVector products = defaultProducts();

    for (auto& product : products)
    {
        product.setProductHasChanged(false);
    }

    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    std::string everest_installer = "/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component/install.sh";
    std::string plugin_installer = "/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR/install.sh";
    EXPECT_CALL(fileSystemMock, exists(everest_installer)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory(everest_installer)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(everest_installer)).Times(1);
    EXPECT_CALL(fileSystemMock, exists(plugin_installer)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory(plugin_installer)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(plugin_installer)).Times(1);
    std::vector<std::string> emptyFileList;
    std::string uninstallPath = "/opt/sophos-spl/base/update/var/installedproducts";
    EXPECT_CALL(fileSystemMock, isDirectory(uninstallPath)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, listFiles(uninstallPath)).WillOnce(Return(emptyFileList));
    EXPECT_CALL(fileSystemMock, exists("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, readLines("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(defaultOverrideSettings()));


    setupFileVersionCalls(fileSystemMock, "PRODUCT_VERSION = 1.1.3.703", "PRODUCT_VERSION = 1.1.3.703");

    int counter = 0;

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&counter]() {
      if (counter++ == 0)
      {
          auto mockProcess = new StrictMock<MockProcess>();
          EXPECT_CALL(*mockProcess, exec(HasSubstr("ServerProtectionLinux-Base-component/install.sh"), _, _)).Times(1);
          EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
          EXPECT_CALL(*mockProcess, output()).WillOnce(Return("installing base"));
          EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
          return std::unique_ptr<Common::Process::IProcess>(mockProcess);
      }
      else
      {
          auto mockProcess = new StrictMock<MockProcess>();
          EXPECT_CALL(*mockProcess, exec(HasSubstr("ServerProtectionLinux-Plugin-EDR/install.sh"), _, _)).Times(1);
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
    EXPECT_CALL(mock, tryConnect(_, _, _)).WillOnce(Return(true)); // successful tryConnect call
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primary")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primary")).WillOnce(Return(true));
    EXPECT_CALL(mock, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(mock, distribute());
    EXPECT_CALL(mock, purge());
    EXPECT_CALL(mock, setWillInstall(_)).Times(1);
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    EXPECT_CALL(mock, getProducts()).WillOnce(Return(products));
    EXPECT_CALL(mock, getSourceURL());
    EXPECT_CALL(mock, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(mock, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));
    Sdds3SimplifiedDownloadReport expectedDownloadReport{ SulDownloader::suldownloaderdata::RepositoryStatus::SUCCESS,
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
    SULDownloaderSdds3Test,
    runSULDownloader_ListOfSubscriptionsWhichDontMatchPreviousConfigResultsInFullSuccessfulUpdate)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(2, 2, 0);
    MockSdds3Repository& mock = repositoryMocked();
    DownloadedProductVector products = defaultProducts();

    for (auto& product : products)
    {
        product.setProductHasChanged(false);
    }
    std::string everest_installer = "/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component/install.sh";
    std::string plugin_installer = "/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR/install.sh";
    EXPECT_CALL(fileSystemMock, exists(everest_installer)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory(everest_installer)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(everest_installer)).Times(1);
    EXPECT_CALL(fileSystemMock, exists(plugin_installer)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory(plugin_installer)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(plugin_installer)).Times(1);
    std::vector<std::string> emptyFileList;
    std::string uninstallPath = "/opt/sophos-spl/base/update/var/installedproducts";
    EXPECT_CALL(fileSystemMock, isDirectory(uninstallPath)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, listFiles(uninstallPath)).WillOnce(Return(emptyFileList));
    EXPECT_CALL(fileSystemMock, exists("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, readLines("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(defaultOverrideSettings()));


    setupFileVersionCalls(fileSystemMock, "PRODUCT_VERSION = 1.1.3.703", "PRODUCT_VERSION = 1.1.3.703");

    int counter = 0;

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&counter]() {
      if (counter++ == 0)
      {
          auto mockProcess = new StrictMock<MockProcess>();
          EXPECT_CALL(*mockProcess, exec(HasSubstr("ServerProtectionLinux-Base-component/install.sh"), _, _)).Times(1);
          EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
          EXPECT_CALL(*mockProcess, output()).WillOnce(Return("installing base"));
          EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
          return std::unique_ptr<Common::Process::IProcess>(mockProcess);
      }
      else
      {
          auto mockProcess = new StrictMock<MockProcess>();
          EXPECT_CALL(*mockProcess, exec(HasSubstr("ServerProtectionLinux-Plugin-EDR/install.sh"), _, _)).Times(1);
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
    EXPECT_CALL(mock, tryConnect(_, _, _)).WillOnce(Return(true)); // successful tryConnect call
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primary")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primary")).WillOnce(Return(true));
    EXPECT_CALL(mock, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(mock, distribute());
    EXPECT_CALL(mock, purge());
    EXPECT_CALL(mock, setWillInstall(_)).Times(1);
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    EXPECT_CALL(mock, getProducts()).WillOnce(Return(products));
    EXPECT_CALL(mock, getSourceURL());
    EXPECT_CALL(mock, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(mock, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));
    Sdds3SimplifiedDownloadReport expectedDownloadReport{ SulDownloader::suldownloaderdata::RepositoryStatus::SUCCESS,
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

    auto actualReport = SulDownloader::runSULDownloader(configurationData, previousConfigurationData, previousDownloadReport);
    EXPECT_FALSE(actualReport.isSupplementOnlyUpdate());

    EXPECT_PRED_FORMAT2(
        downloadReportSimilar,
        expectedDownloadReport,
        actualReport);
}

TEST_F( // NOLINT
    SULDownloaderSdds3Test,
    runSULDownloader_ListOfFeaturesWhichDontMatchPreviousConfigResultsInFullSuccessfulUpdate)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(2, 2, 0);
    MockSdds3Repository& mock = repositoryMocked();
    DownloadedProductVector products = defaultProducts();

    for (auto& product : products)
    {
        product.setProductHasChanged(false);
    }

    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    std::string everest_installer = "/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component/install.sh";
    std::string plugin_installer = "/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR/install.sh";
    EXPECT_CALL(fileSystemMock, exists(everest_installer)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory(everest_installer)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(everest_installer)).Times(1);
    EXPECT_CALL(fileSystemMock, exists(plugin_installer)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory(plugin_installer)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(plugin_installer)).Times(1);
    std::vector<std::string> emptyFileList;
    std::string uninstallPath = "/opt/sophos-spl/base/update/var/installedproducts";
    EXPECT_CALL(fileSystemMock, isDirectory(uninstallPath)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, listFiles(uninstallPath)).WillOnce(Return(emptyFileList));
    EXPECT_CALL(fileSystemMock, exists("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, readLines("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(defaultOverrideSettings()));


    setupFileVersionCalls(fileSystemMock, "PRODUCT_VERSION = 1.1.3.703", "PRODUCT_VERSION = 1.1.3.703");

    int counter = 0;

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&counter]() {
      if (counter++ == 0)
      {
          auto mockProcess = new StrictMock<MockProcess>();
          EXPECT_CALL(*mockProcess, exec(HasSubstr("ServerProtectionLinux-Base-component/install.sh"), _, _)).Times(1);
          EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
          EXPECT_CALL(*mockProcess, output()).WillOnce(Return("installing base"));
          EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
          return std::unique_ptr<Common::Process::IProcess>(mockProcess);
      }
      else
      {
          auto mockProcess = new StrictMock<MockProcess>();
          EXPECT_CALL(*mockProcess, exec(HasSubstr("ServerProtectionLinux-Plugin-EDR/install.sh"), _, _)).Times(1);
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
    EXPECT_CALL(mock, tryConnect(_, _, _)).WillOnce(Return(true)); // successful tryConnect call
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primary")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primary")).WillOnce(Return(true));
    EXPECT_CALL(mock, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(mock, distribute());
    EXPECT_CALL(mock, purge());
    EXPECT_CALL(mock, setWillInstall(_)).Times(1);
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    EXPECT_CALL(mock, getProducts()).WillOnce(Return(products));
    EXPECT_CALL(mock, getSourceURL());
    EXPECT_CALL(mock, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(mock, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));
    Sdds3SimplifiedDownloadReport expectedDownloadReport{ SulDownloader::suldownloaderdata::RepositoryStatus::SUCCESS,
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
    SULDownloaderSdds3Test,
    runSULDownloader_previousProductChangesShouldResultInSuccessfulFullUpdateAndProduceValidSuccessReport)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(1, 2, 0);
    MockSdds3Repository& mock = repositoryMocked();
    DownloadedProductVector products = defaultProducts();

    for (auto& product : products)
    {
        product.setProductHasChanged(false);
    }
    std::string everest_installer = "/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component/install.sh";
    std::string plugin_installer = "/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR/install.sh";
    EXPECT_CALL(fileSystemMock, exists(everest_installer)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory(everest_installer)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(everest_installer)).Times(1);
    EXPECT_CALL(fileSystemMock, exists(plugin_installer)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory(plugin_installer)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(plugin_installer)).Times(1);
    std::vector<std::string> emptyFileList;
    std::string uninstallPath = "/opt/sophos-spl/base/update/var/installedproducts";
    EXPECT_CALL(fileSystemMock, isDirectory(uninstallPath)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, listFiles(uninstallPath)).WillOnce(Return(emptyFileList));
    EXPECT_CALL(fileSystemMock, exists("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, readLines("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(defaultOverrideSettings()));


    setupFileVersionCalls(fileSystemMock, "PRODUCT_VERSION = 1.1.3.703", "PRODUCT_VERSION = 1.1.3.703");

    int counter = 0;

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&counter]() {
      if (counter++ == 0)
      {
          auto mockProcess = new StrictMock<MockProcess>();
          EXPECT_CALL(*mockProcess, exec(HasSubstr("ServerProtectionLinux-Base-component/install.sh"), _, _)).Times(1);
          EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
          EXPECT_CALL(*mockProcess, output()).WillOnce(Return("installing base"));
          EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
          return std::unique_ptr<Common::Process::IProcess>(mockProcess);
      }
      else
      {
          auto mockProcess = new StrictMock<MockProcess>();
          EXPECT_CALL(*mockProcess, exec(HasSubstr("ServerProtectionLinux-Plugin-EDR/install.sh"), _, _)).Times(1);
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
    EXPECT_CALL(mock, tryConnect(_, _, _)).WillOnce(Return(true)); // successful tryConnect call
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primary")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primary")).WillOnce(Return(true));
    EXPECT_CALL(mock, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(mock, distribute());
    EXPECT_CALL(mock, purge());
    EXPECT_CALL(mock, setWillInstall(_)).Times(2);
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    EXPECT_CALL(mock, getProducts()).Times(2).WillRepeatedly(Return(products));
    EXPECT_CALL(mock, getSourceURL());
    EXPECT_CALL(mock, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(mock, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));
    Sdds3SimplifiedDownloadReport expectedDownloadReport{ SulDownloader::suldownloaderdata::RepositoryStatus::SUCCESS,
                                                     "",
                                                     productReports,
                                                     true,
                                                     productsInfo({ products[0], products[1] }) };

    ConfigurationData configurationData = configData(defaultSettings());
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
    SULDownloaderSdds3Test,
    runSULDownloader_WithUpdateConfigDataMatchingWarehouseSynchronizationResultingInNoUpdateNeededShouldCreateValidSuccessReport)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(1, 2, 0);
    MockSdds3Repository& mock = repositoryMocked();
    DownloadedProductVector products = defaultProducts();
    ProductReportVector productReports = defaultProductReports();

    for (auto& product : products)
    {
        product.setProductHasChanged(false);
    }

    EXPECT_CALL(mock, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(mock, tryConnect(_, false, _)).WillOnce(Return(true)); // successful tryConnect call
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primary")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primary")).WillOnce(Return(true));
    EXPECT_CALL(mock, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(mock, distribute());
    EXPECT_CALL(mock, purge());
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    EXPECT_CALL(mock, getProducts()).Times(2).WillRepeatedly(Return(products));
    EXPECT_CALL(mock, getSourceURL());
    EXPECT_CALL(mock, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(mock, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));

    std::vector<std::string> emptyFileList;
    std::string uninstallPath = "/opt/sophos-spl/base/update/var/installedproducts";
    EXPECT_CALL(fileSystemMock, isDirectory(uninstallPath)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, listFiles(uninstallPath)).WillOnce(Return(emptyFileList));
    EXPECT_CALL(fileSystemMock, exists("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, readLines("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(defaultOverrideSettings()));


    setupFileVersionCalls(fileSystemMock, "PRODUCT_VERSION = 1.1.3.703", "PRODUCT_VERSION = 1.1.3.703");

    Sdds3SimplifiedDownloadReport expectedDownloadReport{ SulDownloader::suldownloaderdata::RepositoryStatus::SUCCESS,
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
    SULDownloaderSdds3Test,
    runSULDownloader_supplement_only_WithUpdateConfigDataMatchingWarehouseSynchronizationResultingInNoUpdateNeededShouldCreateValidSuccessReport)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(1, 2, 0);
    MockSdds3Repository& mock = repositoryMocked();
    DownloadedProductVector products = defaultProducts();
    ProductReportVector productReports = defaultProductReports();
    for (auto& product : products)
    {
        product.setProductHasChanged(false);
    }

    EXPECT_CALL(mock, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(mock, tryConnect(_, true, _)).WillOnce(Return(true)); // successful tryConnect call
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primary")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primary")).WillOnce(Return(true));
    EXPECT_CALL(mock, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(mock, distribute());
    EXPECT_CALL(mock, purge());
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    EXPECT_CALL(mock, getProducts()).Times(2).WillRepeatedly(Return(products));
    EXPECT_CALL(mock, getSourceURL());
    EXPECT_CALL(mock, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(mock, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));

    std::vector<std::string> emptyFileList;
    std::string uninstallPath = "/opt/sophos-spl/base/update/var/installedproducts";
    EXPECT_CALL(fileSystemMock, isDirectory(uninstallPath)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, listFiles(uninstallPath)).WillOnce(Return(emptyFileList));
    EXPECT_CALL(fileSystemMock, exists("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, readLines("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(defaultOverrideSettings()));


    setupFileVersionCalls(fileSystemMock, "PRODUCT_VERSION = 1.1.3.703", "PRODUCT_VERSION = 1.1.3.703");

    Sdds3SimplifiedDownloadReport expectedDownloadReport{ SulDownloader::suldownloaderdata::RepositoryStatus::SUCCESS,
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

    auto actualDownloadReport = SulDownloader::runSULDownloader(configurationData, previousConfigurationData, previousDownloadReport, true);
    EXPECT_TRUE(actualDownloadReport.isSupplementOnlyUpdate());

    EXPECT_PRED_FORMAT2(
        downloadReportSimilar,
        expectedDownloadReport,
        actualDownloadReport);
}

TEST_F( // NOLINT
    SULDownloaderSdds3Test,
    runSULDownloader_checkLogVerbosityVERBOSE)
{
    auto& fileSystem = setupFileSystemAndGetMock(1, 1, 0);
    EXPECT_CALL(fileSystem, isFile("/etc/ssl/certs/ca-certificates.crt")).WillOnce(Return(false));
    EXPECT_CALL(fileSystem, isFile("/etc/pki/tls/certs/ca-bundle.crt")).WillOnce(Return(false));
    EXPECT_CALL(fileSystem, isFile("/etc/ssl/ca-bundle.pem")).WillOnce(Return(false));
    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();
    auto settings = defaultSettings();
    settings.clear_sophosurls();
    settings.add_sophosurls("http://localhost/latest/donotexits");
    settings.set_loglevel(ConfigurationSettings::VERBOSE);
    suldownloaderdata::ConfigurationData configurationData = configData(settings);
    suldownloaderdata::ConfigurationData previousConfigurationData;
    configurationData.verifySettingsAreValid();
    DownloadReport previousDownloadReport = DownloadReport::Report("Not assigned");
    EXPECT_CALL(fileSystem, exists("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystem, isFile("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystem, readLines("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(defaultOverrideSettings()));


    auto downloadReport =
        SulDownloader::runSULDownloader(configurationData, previousConfigurationData, previousDownloadReport);
    std::string output = testing::internal::GetCapturedStdout();
    std::string errStd = testing::internal::GetCapturedStderr();
    ASSERT_THAT(errStd, ::testing::HasSubstr("Failed to connect to repository"));
}

TEST_F( // NOLINT
    SULDownloaderSdds3Test,
    runSULDownloader_checkLogVerbosityNORMAL)
{
    auto& fileSystem = setupFileSystemAndGetMock(1,1, 0);
    EXPECT_CALL(fileSystem, isFile("/etc/ssl/certs/ca-certificates.crt")).WillOnce(Return(false));
    EXPECT_CALL(fileSystem, isFile("/etc/pki/tls/certs/ca-bundle.crt")).WillOnce(Return(false));
    EXPECT_CALL(fileSystem, isFile("/etc/ssl/ca-bundle.pem")).WillOnce(Return(false));
    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();
    auto settings = defaultSettings();
    settings.clear_sophosurls();
    settings.add_sophosurls("http://localhost/latest/donotexits");
    settings.set_loglevel(ConfigurationSettings::NORMAL);
    ConfigurationData configurationData = configData(settings);
    ConfigurationData previousConfigurationData;
    configurationData.verifySettingsAreValid();
    DownloadReport previousDownloadReport = DownloadReport::Report("Not assigned");
    EXPECT_CALL(fileSystem, exists("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystem, isFile("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystem, readLines("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(defaultOverrideSettings()));

    auto downloadReport =
        SulDownloader::runSULDownloader(configurationData, previousConfigurationData, previousDownloadReport);
    std::string output = testing::internal::GetCapturedStdout();
    std::string errStd = testing::internal::GetCapturedStderr();

    ASSERT_THAT(output, ::testing::Not(::testing::HasSubstr("Proxy used was")));
    ASSERT_THAT(errStd, ::testing::Not(::testing::HasSubstr("Proxy used was")));
    ASSERT_THAT(errStd, ::testing::HasSubstr("Failed to connect to repository"));
}

TEST_F( // NOLINT
    SULDownloaderSdds3Test,
    runSULDownloader_failureToPurgeSDDS2CacheDoesNotHaltSync)
{
    auto& fileSystemMock = setupFileSystemAndGetMock();
    MockSdds3Repository& mock = repositoryMocked();

    auto products = defaultProducts();
    products[0].setProductHasChanged(false);
    products[1].setProductHasChanged(false);

    EXPECT_CALL(mock, tryConnect(_, _, _)).WillOnce(Return(true));
    EXPECT_CALL(mock, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Throw(Common::FileSystem::IFileSystemException("error")));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(mock, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(mock, distribute());
    EXPECT_CALL(mock, purge());
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    EXPECT_CALL(mock, getProducts()).Times(2).WillRepeatedly(Return(products));
    EXPECT_CALL(mock, getSourceURL());
    EXPECT_CALL(mock, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(mock, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));

    setupFileVersionCalls(fileSystemMock, "PRODUCT_VERSION = 1.1.3.703", "PRODUCT_VERSION = 1.1.3.703");

    TimeTracker timeTracker;
    timeTracker.setStartTime(std::time_t(0));
    timeTracker.setFinishedTime(std::time_t(0));
    DownloadReport downloadReport =
        DownloadReport::Report("", products, {}, {}, &timeTracker, DownloadReport::VerifyState::VerifyCorrect);
    std::string previousJsonReport = DownloadReport::fromReport(downloadReport);
    std::string previousReportFilename = "update_report-previous.json";
    std::vector<std::string> previousReportFileList = { previousReportFilename };
    std::vector<std::string> emptyFileList;

    EXPECT_CALL(fileSystemMock, readFile("/dir/input.json")).WillOnce(Return(jsonSettings(defaultSettings())));
    EXPECT_CALL(fileSystemMock, isFile("/dir/previous_update_config.json")).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, isFile("/dir/supplement_only.marker")).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, isDirectory("/dir/output.json")).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, isDirectory("/dir")).WillOnce(Return(true));

    EXPECT_CALL(fileSystemMock, listFiles("/dir")).WillOnce(Return(previousReportFileList));
    EXPECT_CALL(fileSystemMock, readFile(previousReportFilename)).WillOnce(Return(previousJsonReport));
    EXPECT_CALL(fileSystemMock, exists("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, readLines("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(defaultOverrideSettings()));

    setupExpectanceWriteAtomically(
        fileSystemMock,
        SulDownloader::suldownloaderdata::toString(SulDownloader::suldownloaderdata::RepositoryStatus::SUCCESS));

    std::string uninstallPath = "/opt/sophos-spl/base/update/var/installedproducts";
    EXPECT_CALL(fileSystemMock, isDirectory(uninstallPath)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, listFiles(uninstallPath)).WillOnce(Return(emptyFileList));

    Common::ProcessImpl::ArgcAndEnv args("SulDownloader", { "/dir/input.json", "/dir/output.json" }, {});

    EXPECT_EQ(SulDownloader::main_entry(3, args.argc()), 0);
}

TEST_F(SULDownloaderSdds3Test,updateFailsIfNoJWTToken) // NOLINT
{
    setupFileSystemAndGetMock(1, 0, 0);
    testing::internal::CaptureStderr();

    auto settings = defaultSettings();
    settings.set_loglevel(ConfigurationSettings::NORMAL);
    settings.mutable_jwtoken()->assign("");
    ConfigurationData configurationData = configData(settings);
    ConfigurationData previousConfigurationData;
    configurationData.verifySettingsAreValid();
    DownloadReport previousDownloadReport = DownloadReport::Report("Not assigned");

    auto report =
        SulDownloader::runSULDownloader(configurationData, previousConfigurationData, previousDownloadReport);

    //We wont writeInstalledFeatures if return code is non zero
    EXPECT_EQ(report.getStatus(), suldownloaderdata::RepositoryStatus::DOWNLOADFAILED);

    std::string errStd = testing::internal::GetCapturedStderr();
    ASSERT_THAT(errStd, ::testing::HasSubstr("Failed to update because JWToken was empty"));
}

TEST_F(SULDownloaderSdds3Test,updateFailsIfOldVersion) // NOLINT
{
    setupFileSystemAndGetMock(1, 0, 0);
    testing::internal::CaptureStderr();

    auto settings = defaultSettings();
    auto primarySubscription = ProductSubscription("rigidname", "baseversion", "tag", "2020");
    settings.set_loglevel(ConfigurationSettings::NORMAL);
    ConfigurationData configurationData = configData(settings);
    configurationData.setPrimarySubscription(primarySubscription);
    ConfigurationData previousConfigurationData;
    configurationData.verifySettingsAreValid();
    DownloadReport previousDownloadReport = DownloadReport::Report("Not assigned");

    auto report =
        SulDownloader::runSULDownloader(configurationData, previousConfigurationData, previousDownloadReport);

    //We wont writeInstalledFeatures if return code is non zero
    EXPECT_EQ(report.getStatus(), suldownloaderdata::RepositoryStatus::DOWNLOADFAILED);

    std::string errStd = testing::internal::GetCapturedStderr();
    ASSERT_THAT(errStd, ::testing::HasSubstr("The requested fixed version is not available on SDDS3"));
}

// Suldownloader WriteInstalledFeatures()
class TestSuldownloaderWriteInstalledFeaturesFunction: public LogOffInitializedTests{};

TEST_F(TestSuldownloaderWriteInstalledFeaturesFunction, featuresAreWrittenToJsonFile) // NOLINT
{
    setupInstalledFeaturesPermissions();
    auto filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(
        *filesystemMock,
        writeFile(
            Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath(),
            "[\"feature1\",\"feature2\"]"));
    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    std::vector<std::string> features = { "feature1", "feature2" };
    SulDownloader::writeInstalledFeatures(features);
}

TEST_F(TestSuldownloaderWriteInstalledFeaturesFunction, emptyListOfFeaturesAreWrittenToJsonFile) // NOLINT
{
    setupInstalledFeaturesPermissions();
    auto filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(
        *filesystemMock,
        writeFile(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath(), "[]"));
    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    std::vector<std::string> features = {};
    SulDownloader::writeInstalledFeatures(features);
}

TEST_F(TestSuldownloaderWriteInstalledFeaturesFunction, noThrowExpectedOnFileSystemError) // NOLINT
{
    auto filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(
        *filesystemMock,
        writeFile(
            Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath(),
            "[\"feature1\",\"feature2\"]"))
        .WillOnce(Throw(Common::FileSystem::IFileSystemException("error")));

    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    std::vector<std::string> features = { "feature1", "feature2" };
    EXPECT_NO_THROW(SulDownloader::writeInstalledFeatures(features));
}