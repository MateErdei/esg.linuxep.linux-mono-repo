// Copyright 2018-2023 Sophos Limited. All rights reserved.

/**
 * Component tests to SULDownloader mocking out Sdds3Repository
 */

#include "ConfigurationSettings.pb.h"
#include "MockSdds3Repository.h"
#include "MockVersig.h"
#include "TestSdds3RepositoryHelper.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/Logging/ConsoleLoggingSetup.h"
#include "Common/ProcessImpl/ArgcAndEnv.h"
#include "Common/ProcessImpl/ProcessImpl.h"
#include "Common/ProtobufUtil/MessageUtility.h"
#include "Common/UtilityImpl/ProjectNames.h"
#include "SulDownloader/SulDownloader.h"
#include "SulDownloader/suldownloaderdata/ConfigurationData.h"
#include "SulDownloader/suldownloaderdata/DownloadReport.h"
#include "SulDownloader/suldownloaderdata/SulDownloaderException.h"
#include "SulDownloader/suldownloaderdata/TimeTracker.h"
#include "SulDownloader/suldownloaderdata/VersigImpl.h"
#include "tests/Common/FileSystemImpl/MockPidLockFileUtils.h"
#include "tests/Common/Helpers/FilePermissionsReplaceAndRestore.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/MemoryAppender.h"
#include "tests/Common/Helpers/MockFilePermissions.h"
#include "tests/Common/Helpers/MockFileSystem.h"
#include "tests/Common/Helpers/MockProcess.h"
#include "tests/Common/UtilityImpl/TestStringGenerator.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace Common::Policy;
using namespace SulDownloader::suldownloaderdata;

using PolicyProto::ConfigurationSettings;

struct Sdds3SimplifiedDownloadReport
{
    RepositoryStatus Status;
    std::string Description;
    std::vector<ProductReport> Products;
    bool shouldContainSyncTime;
    std::vector<ProductInfo> WarehouseComponents;
};

namespace
{
    using DownloadedProductVector = std::vector<DownloadedProduct>;
    using ProductReportVector = std::vector<ProductReport>;

    void setupInstalledFeaturesPermissions()
    {
        auto mockFilePermissions = std::make_unique<StrictMock<MockFilePermissions>>();
        EXPECT_CALL(*mockFilePermissions, chown(::testing::HasSubstr("installed_features.json"), sophos::updateSchedulerUser(), sophos::group()));
        EXPECT_CALL(*mockFilePermissions, chmod(::testing::HasSubstr("installed_features.json"), S_IRUSR | S_IWUSR | S_IRGRP));
        Tests::replaceFilePermissions(std::move(mockFilePermissions));
    }

} // namespace

class TestSulDownloaderSdds3Base
{
public:
    static std::vector<std::string> defaultOverrideSettings()
    {
        std::vector<std::string> lines = {
            "URLS = http://127.0.0.1:8080"};
        return lines;
    }
    static ConfigurationSettings defaultSettings()
    {
        ConfigurationSettings settings;

        settings.add_sophoscdnurls("http://ostia.eng.sophosinvalid/latest/Virt-vShieldInvalid");
        settings.mutable_sophossusurl()->assign("https://sus.sophosupd.co");
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

    ConfigurationSettings newFeatureSettings(const std::string& feature = "SOME_FEATURE")
    {
        ConfigurationSettings settings = defaultSettings();
        settings.add_features(feature);
        return settings;
    }

    ConfigurationSettings newSubscriptionSettings(const std::string& rigidname = "Plugin_1")
    {
        ConfigurationSettings settings = defaultSettings();
        auto* proto_subscriptions = settings.mutable_products();

        PolicyProto::ConfigurationSettings_Subscription proto_subscription;

        proto_subscription.set_rigidname(rigidname);
        proto_subscription.set_baseversion("");
        proto_subscription.set_tag("RECOMMENDED");
        proto_subscription.set_fixedversion("");

        proto_subscriptions->Add(dynamic_cast<PolicyProto::ConfigurationSettings_Subscription&&>(proto_subscription));

        return settings;
    }

    std::string jsonSettings(const ConfigurationSettings& configSettings)
    {
        return Common::ProtobufUtil::MessageUtility::protoBuf2Json(configSettings);
    }

    UpdateSettings configData(const ConfigurationSettings& configSettings)
    {
        std::string json_output = jsonSettings(configSettings);
        return ConfigurationData::fromJsonSettings(json_output);
    }

    DownloadedProductVector defaultProducts()
    {
        DownloadedProductVector products;
        for (auto& metadata : defaultMetadata())
        {
            DownloadedProduct product(metadata);
            product.setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary");
            products.push_back(product);
        }
        products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
        products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
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

    std::vector<ProductMetadata> defaultMetadata()
    {
        ProductMetadata base;
        base.setDefaultHomePath("ServerProtectionLinux-Base-component");
        base.setLine("ServerProtectionLinux-Base-component");
        base.setName("ServerProtectionLinux-Base-component-Product");
        base.setVersion("10.2.3");
        base.setTags({ { "RECOMMENDED", "10", "Base-label" } });

        ProductMetadata plugin;
        plugin.setDefaultHomePath("ServerProtectionLinux-Plugin-EDR");
        plugin.setLine("ServerProtectionLinux-Plugin-EDR");
        plugin.setName("ServerProtectionLinux-Plugin-EDR-Product");
        plugin.setVersion("10.3.5");
        plugin.setTags({ { "RECOMMENDED", "10", "Plugin-label" } });

        return std::vector<ProductMetadata>{ base, plugin };
    }

    ProductReportVector defaultProductReports()
    {
        ProductReport base;
        base.name = "ServerProtectionLinux-Base-component-Product";
        base.rigidName = "ServerProtectionLinux-Base-component";
        base.downloadedVersion = "10.2.3";
        base.productStatus = ProductReport::ProductStatus::UpToDate;
        ProductReport plugin;
        plugin.name = "ServerProtectionLinux-Plugin-EDR-Product";
        plugin.rigidName = "ServerProtectionLinux-Plugin-EDR";
        plugin.downloadedVersion = "10.3.5";
        plugin.productStatus = ProductReport::ProductStatus::UpToDate;
        return ProductReportVector{ base, plugin };
    }

    /**
 * @return BORROWED reference to MockFileSystem
     */
    MockFileSystem& setupFileSystemAndGetMock(int expectCallCount = 1, int expectCurrentProxy = 1, int expectedInstalledFeatures = 1, std::string installedFeatures = R"(["CORE"])")
    {
        Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL, "/opt/sophos-spl");

        auto filesystemMock = std::make_unique<MockFileSystem>();
        EXPECT_CALL(*filesystemMock, isFile).Times(AnyNumber());
        EXPECT_CALL(*filesystemMock, isDirectory).Times(AnyNumber());
        EXPECT_CALL(*filesystemMock, removeFile(_, _)).Times(AnyNumber());
        if (expectCallCount > 0)
        {
            EXPECT_CALL(*filesystemMock, isDirectory("/opt/sophos-spl"))
                .Times(expectCallCount)
                .WillRepeatedly(Return(true));
            EXPECT_CALL(*filesystemMock, isDirectory("/opt/sophos-spl/base/update/cache/"))
                .Times(expectCallCount)
                .WillRepeatedly(Return(true));
        }
        else
        {
            ON_CALL(*filesystemMock, isDirectory("/opt/sophos-spl")).WillByDefault(Return(true));
            ON_CALL(*filesystemMock, isDirectory("/opt/sophos-spl/base/update/cache/")).WillByDefault(Return(true));
        }

        EXPECT_CALL(*filesystemMock, exists(_)).WillRepeatedly(Return(true));
        EXPECT_CALL(*filesystemMock, currentWorkingDirectory()).WillRepeatedly(Return("/opt/sophos-spl/base/bin"));
        EXPECT_CALL(*filesystemMock, isFile("/opt/sophos-spl/base/etc/savedproxy.config")).WillRepeatedly(Return(false));
        std::string currentProxyFilePath =
            Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFilePath();
        if (expectCurrentProxy > 0)
        {
            EXPECT_CALL(*filesystemMock, isFile(currentProxyFilePath))
                .Times(expectCurrentProxy)
                .WillRepeatedly(Return(false));
        }
        std::string installedFeaturesFile = Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath();
        EXPECT_CALL(*filesystemMock, writeFile(::testing::HasSubstr("installed_features.json"), installedFeatures)).Times(expectedInstalledFeatures);

        setupExpectanceWriteProductUpdate(*filesystemMock);

        auto* pointer = filesystemMock.get();
        m_replacer.replace(std::move(filesystemMock));
        return *pointer;
    }

    void setupFileVersionCalls(MockFileSystem& fileSystemMock, const std::string& currentVersion, const std::string& newVersion)
    {
        setupBaseVersionFileCalls(fileSystemMock, currentVersion, newVersion);
        setupPluginVersionFileCalls(fileSystemMock, currentVersion, newVersion);
    }

    static void setupBaseVersionFileCalls(MockFileSystem& fileSystemMock, const std::string& currentVersion, const std::string& newVersion)
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
        EXPECT_CALL(mockFileSystem, moveFile(_, Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath())).Times(installedFeaturesWritesExpected);

        auto mockFilePermissions = std::make_unique<StrictMock<MockFilePermissions>>();
        EXPECT_CALL(*mockFilePermissions, chown(testing::HasSubstr("/opt/sophos-spl/tmp"), sophos::updateSchedulerUser(), "root"));
        mode_t expectedFilePermissions = S_IRUSR | S_IWUSR;
        EXPECT_CALL(*mockFilePermissions, chmod(testing::HasSubstr("/opt/sophos-spl/tmp"), expectedFilePermissions));

        EXPECT_CALL(*mockFilePermissions, chown(testing::HasSubstr("installed_features.json"), sophos::updateSchedulerUser(), sophos::group())).Times(installedFeaturesWritesExpected);
        EXPECT_CALL(*mockFilePermissions, chmod(testing::HasSubstr("installed_features.json"), S_IRUSR | S_IWUSR | S_IRGRP)).Times(installedFeaturesWritesExpected);

        EXPECT_CALL(*mockFilePermissions, getGroupId(sophos::group())).WillRepeatedly(Return(123));
        EXPECT_CALL(*mockFilePermissions, chown(Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderLockFilePath(), "root", sophos::group())).WillRepeatedly(Return());
        EXPECT_CALL(*mockFilePermissions, chmod(Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderLockFilePath(), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)).WillRepeatedly(Return());

        Tests::replaceFilePermissions(std::move(mockFilePermissions));
    }

    ::testing::AssertionResult downloadReportSimilar(
        const char* m_expr,
        const char* n_expr,
        const Sdds3SimplifiedDownloadReport& expected,
        const DownloadReport& resulted)
    {
        std::stringstream s;
        s << m_expr << " and " << n_expr << " failed: ";

        if (expected.Status != resulted.getStatus())
        {
            return ::testing::AssertionFailure()
                   << s.str()
                   << " status differ: \n expected: " << toString(expected.Status)
                   << "\n result: " << toString(resulted.getStatus());
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

        return listProductInfoIsEquivalent(m_expr, n_expr, expected.WarehouseComponents, resulted.getRepositoryComponents());
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

    std::unique_ptr<Common::Process::IProcess> mockBaseInstall(MockFileSystem& mockFileSystem, int exitCode = 0)
        {
        auto mockProcess = std::make_unique<StrictMock<MockProcess>>();
        EXPECT_CALL(*mockProcess, exec(HasSubstr("ServerProtectionLinux-Base-component/install.sh"), _, _)).Times(1);
        EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
        EXPECT_CALL(*mockProcess, output()).WillOnce(Return("installing base"));
        EXPECT_CALL(mockFileSystem, writeFile(_,"installing base")).WillOnce(Return());
        EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(exitCode));
        return mockProcess;
    }

    std::unique_ptr<Common::Process::IProcess> mockEDRInstall(MockFileSystem& mockFileSystem, int exitCode = 0)
    {
        auto mockProcess = std::make_unique<StrictMock<MockProcess>>();
        EXPECT_CALL(*mockProcess, exec(HasSubstr("ServerProtectionLinux-Plugin-EDR/install.sh"), _, _)).Times(1);
        EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
        EXPECT_CALL(*mockProcess, output()).WillOnce(Return("installing plugin"));
        EXPECT_CALL(mockFileSystem, writeFile(_,"installing plugin")).WillOnce(Return());
        EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(exitCode));
        return mockProcess;
    }

    std::unique_ptr<Common::Process::IProcess> mockSystemctlStatus(int exitCode = 4)
    {
        auto mockProcess = std::make_unique<StrictMock<MockProcess>>();
        std::vector<std::string> stop_args = {"status","sophos-spl"};
        EXPECT_CALL(*mockProcess, exec("systemctl", stop_args)).Times(1);
        EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
        EXPECT_CALL(*mockProcess, output()).WillOnce(Return("watchdog running!"));
        EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(exitCode));
        return mockProcess;
    }

    std::unique_ptr<Common::Process::IProcess> mockSystemctlStop(int exitCode = 0)
    {
        auto mockProcess = std::make_unique<StrictMock<MockProcess>>();
        std::vector<std::string> start_args = {"stop","sophos-spl"};
        EXPECT_CALL(*mockProcess, exec("systemctl", start_args)).Times(1);
        EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
        EXPECT_CALL(*mockProcess, output()).WillOnce(Return(""));
        EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(exitCode));
        return mockProcess;
    }
    std::unique_ptr<Common::Process::IProcess> mockSystemctlStart(int exitCode = 0)
    {
        auto mockProcess = std::make_unique<StrictMock<MockProcess>>();
        std::vector<std::string> start_args = {"start","sophos-spl"};
        EXPECT_CALL(*mockProcess, exec("systemctl", start_args)).Times(1);
        EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
        EXPECT_CALL(*mockProcess, output()).WillOnce(Return(""));
        EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(exitCode));
        return mockProcess;
    }

    std::unique_ptr<Common::Process::IProcess> mockIsEdrRunning(int exitCode = 0)
    {
        auto mockProcess = std::make_unique<StrictMock<MockProcess>>();
        std::vector<std::string> start_args = {"isrunning","edr"};
        EXPECT_CALL(*mockProcess, exec(HasSubstr("wdctl"), start_args)).Times(1);
        EXPECT_CALL(*mockProcess, wait(_, _)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
        EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(exitCode));
        return mockProcess;
    }

    std::unique_ptr<Common::Process::IProcess> mockSimpleExec(const std::string& substr, int exitCode = 0)
    {
        auto mockProcess = std::make_unique<StrictMock<MockProcess>>();
        EXPECT_CALL(*mockProcess, exec(HasSubstr(substr), _, _)).Times(1);
        EXPECT_CALL(*mockProcess, output()).Times(1);
        EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(exitCode));
        return mockProcess;
    }

protected:
    std::unique_ptr<MockSdds3Repository> mockSdds3Repo_;
    Tests::ScopedReplaceFileSystem m_replacer;
};

class SULDownloaderSdds3Test : public MemoryAppenderUsingTests, public TestSulDownloaderSdds3Base
{
    Common::Logging::ConsoleLoggingSetup m_consoleLogging;

public:
    SULDownloaderSdds3Test() : MemoryAppenderUsingTests("suldownloader")
    {}
    /**
     * Setup directories and files expected by SULDownloader to enable its execution.
     * Use TempDir
     */
    void SetUp() override
    {
        VersigFactory::instance().replaceCreator([]() {
          auto versig = std::make_unique<NiceMock<MockVersig>>();
          ON_CALL(*versig, verify(_, _)).WillByDefault(Return(IVersig::VerifySignature::SIGNATURE_VERIFIED));
          return versig;
        });

        auto mockPidLockFileUtilsPtr = std::make_unique<NiceMock<MockPidLockFileUtils>>();
        ON_CALL(*mockPidLockFileUtilsPtr, write(_, _, _)).WillByDefault(Return(1));
        Common::FileSystemImpl::replacePidLockUtils(std::move(mockPidLockFileUtilsPtr));

       mockSdds3Repo_ = std::make_unique<NaggyMock<MockSdds3Repository>>();
    }

    /**
     * Remove the temporary directory.
     */
    void TearDown() override
    {
        Common::FileSystemImpl::restorePidLockUtils();
        VersigFactory::instance().restoreCreator();
        TestSdds3RepositoryHelper::restoreSdds3RepositoryFactory();
        Tests::restoreFilePermissions();
        Common::ApplicationConfiguration::restoreApplicationPathManager();
        Tests::restoreFileSystem();
        Test::TearDown();
    }
};

TEST_F(SULDownloaderSdds3Test, configurationDataVerificationOfDefaultSettingsReturnsTrue)
{
    setupFileSystemAndGetMock(1, 0, 0);
    auto confData = configData(defaultSettings());
    confData.verifySettingsAreValid();
    EXPECT_TRUE(confData.isVerified());
}

TEST_F(SULDownloaderSdds3Test, configurationDataVerificationOfSettingsWithESMReturnsTrue)
{
    setupFileSystemAndGetMock(1, 0, 0);
    auto settings = defaultSettings();
    auto esmMutable = settings.mutable_esmversion();
    esmMutable->set_token("token");
    esmMutable->set_name("name");
    auto confData = configData(defaultSettings());

    confData.verifySettingsAreValid();
    EXPECT_TRUE(confData.isVerified());
}

TEST_F(SULDownloaderSdds3Test, main_entry_InvalidArgumentsReturnsTheCorrectErrorCode)
{
    char** argsNotUsed = nullptr;
    EXPECT_EQ(SulDownloader::main_entry(2, argsNotUsed), -1);
    EXPECT_EQ(SulDownloader::main_entry(1, argsNotUsed), -1);
}

TEST_F(SULDownloaderSdds3Test, main_entry_missingInputFileCausesReturnsCorrectErrorCode)
{
    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*filesystemMock, isFile("supplement_only.marker")).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, isDirectory("/some/dir")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, isDirectory("/some/dir/createoutputpath.json")).WillOnce(Return(false));
    std::vector<Path> noOldReports;
    EXPECT_CALL(*filesystemMock, listFiles("/some/dir")).WillOnce(Return(noOldReports));
    EXPECT_CALL(*filesystemMock, readFile("inputfiledoesnotexists.json")).WillRepeatedly(Throw(IFileSystemException("Cannot read file")));
    m_replacer.replace(std::move(filesystemMock));

    char* inputFileDoesNotExist[] = { const_cast<char*>("SulDownloader"),
                                      const_cast<char*>("inputfiledoesnotexists.json"),
                                      const_cast<char*>("/some/dir/createoutputpath.json") };
    EXPECT_EQ(SulDownloader::main_entry(3, inputFileDoesNotExist, std::chrono::milliseconds{1}), -2);
}

TEST_F(SULDownloaderSdds3Test, main_entry_inputFileGivenAsDirReturnsCorrectErrorCode)
{
    auto filesystemMock = std::make_unique<NaggyMock<MockFileSystem>>();
    EXPECT_CALL(*filesystemMock, isDirectory("/opt/sophos-spl/directorypath")).WillOnce(Return(true));
    m_replacer.replace(std::move(filesystemMock));

    // directory can not be replaced by file
    Common::ProcessImpl::ArgcAndEnv args("SulDownloader", { "input.json", "/opt/sophos-spl/directorypath" }, {});

    EXPECT_EQ(SulDownloader::main_entry(3, args.argc()), -2);
}

TEST_F(
    SULDownloaderSdds3Test,
    main_entry_onSuccessWhileForcingUpdateAsPreviousDownloadReportDoesNotExistCreatesReportContainingExpectedSuccessResult)
{
    auto& fileSystemMock = setupFileSystemAndGetMock();

    auto products = defaultProducts();
    products[0].setProductHasChanged(false);
    products[1].setProductHasChanged(false);

    EXPECT_CALL(*mockSdds3Repo_, tryConnect(_, _, _)).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primary")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primary")).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, distribute());
    EXPECT_CALL(*mockSdds3Repo_, purge());
    EXPECT_CALL(*mockSdds3Repo_, setWillInstall(_)).Times(2);
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    EXPECT_CALL(*mockSdds3Repo_, getProducts()).Times(2).WillRepeatedly(Return(products));
    EXPECT_CALL(*mockSdds3Repo_, getSourceURL());
    EXPECT_CALL(*mockSdds3Repo_, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(*mockSdds3Repo_, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));

    TestSdds3RepositoryHelper::replaceSdds3RepositoryCreator([this]() { return std::move(mockSdds3Repo_); });

    std::vector<std::string> emptyFileList;
    EXPECT_CALL(fileSystemMock, readFile("/dir/input.json")).WillOnce(Return(jsonSettings(defaultSettings())));
    EXPECT_CALL(fileSystemMock, isFile("/dir/previous_update_config.json")).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, isFile("/dir/supplement_only.marker")).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, isDirectory("/dir/output.json")).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, isDirectory("/dir")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, listFiles("/dir")).WillOnce(Return(emptyFileList));
    EXPECT_CALL(fileSystemMock, exists("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, readLines("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(defaultOverrideSettings()));
    setupFileVersionCalls(fileSystemMock, "PRODUCT_VERSION = 1.1.3.703", "PRODUCT_VERSION = 1.1.3.703");

    setupExpectanceWriteAtomically(
        fileSystemMock,
        toString(RepositoryStatus::SUCCESS), 1);
    std::string baseInstallPath = "/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component/install.sh";
    EXPECT_CALL(fileSystemMock, isDirectory(baseInstallPath)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(baseInstallPath));

    std::string pluginInstallPath = "/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR/install.sh";
    EXPECT_CALL(fileSystemMock, isDirectory(pluginInstallPath)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(pluginInstallPath));

    std::string uninstallPath = "/opt/sophos-spl/base/update/var/installedproducts";
    EXPECT_CALL(fileSystemMock, isDirectory(uninstallPath)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, listFiles(uninstallPath)).WillOnce(Return(emptyFileList));
    EXPECT_CALL(fileSystemMock, getSystemCommandExecutablePath(_)).WillRepeatedly(Return("systemctl"));

    Common::ProcessImpl::ArgcAndEnv args("SulDownloader", { "/dir/input.json", "/dir/output.json" }, {});

    int counter = 0;
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&counter,this, &fileSystemMock]() {
        if (counter == 0 || counter == 3)
        {
           counter++;
           return mockSystemctlStatus();
        }
      else if (counter == 1)
      {
          counter++;
          return mockBaseInstall(fileSystemMock);
      }
      else if (counter == 2)
      {
          counter++;
          return mockEDRInstall(fileSystemMock);
      }
      else
      {
          counter++;
          return mockSystemctlStart();
      }
    });

    EXPECT_EQ(SulDownloader::main_entry(3, args.argc()), 0);
}

TEST_F(SULDownloaderSdds3Test, main_entry_onSuccessCreatesReportContainingExpectedSuccessResult)
{
    auto& fileSystemMock = setupFileSystemAndGetMock();

    auto products = defaultProducts();
    products[0].setProductHasChanged(false);
    products[1].setProductHasChanged(false);

    EXPECT_CALL(*mockSdds3Repo_, tryConnect(_, _, _)).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primary")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primary")).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, distribute());
    EXPECT_CALL(*mockSdds3Repo_, purge());
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    EXPECT_CALL(*mockSdds3Repo_, getProducts()).Times(2).WillRepeatedly(Return(products));
    EXPECT_CALL(*mockSdds3Repo_, getSourceURL());
    EXPECT_CALL(*mockSdds3Repo_, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(*mockSdds3Repo_, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));
    TestSdds3RepositoryHelper::replaceSdds3RepositoryCreator([this]() { return std::move(mockSdds3Repo_); });

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
        toString(RepositoryStatus::SUCCESS));

    std::string uninstallPath = "/opt/sophos-spl/base/update/var/installedproducts";
    EXPECT_CALL(fileSystemMock, isDirectory(uninstallPath)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, listFiles(uninstallPath)).WillOnce(Return(emptyFileList));

    Common::ProcessImpl::ArgcAndEnv args("SulDownloader", { "/dir/input.json", "/dir/output.json" }, {});

    EXPECT_EQ(SulDownloader::main_entry(3, args.argc()), 0);
}

TEST_F(
    SULDownloaderSdds3Test,
    main_entry_onSuccessCreatesReportContainingExpectedSuccessResultEnsuringInvalidPreviousReportFilesAreIgnored)
{
    auto& fileSystemMock = setupFileSystemAndGetMock();

    auto products = defaultProducts();
    products[0].setProductHasChanged(false);
    products[1].setProductHasChanged(false);

    EXPECT_CALL(*mockSdds3Repo_, tryConnect(_, _, _)).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primary")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primary")).WillOnce(Return(true));

    EXPECT_CALL(*mockSdds3Repo_, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, distribute());
    EXPECT_CALL(*mockSdds3Repo_, purge());
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    EXPECT_CALL(*mockSdds3Repo_, getProducts()).Times(2).WillRepeatedly(Return(products));
    EXPECT_CALL(*mockSdds3Repo_, getSourceURL());
    EXPECT_CALL(*mockSdds3Repo_, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(*mockSdds3Repo_, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));
    TestSdds3RepositoryHelper::replaceSdds3RepositoryCreator([this]() { return std::move(mockSdds3Repo_); });

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
        toString(RepositoryStatus::SUCCESS));

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

TEST_F(
    SULDownloaderSdds3Test,
    main_entry_onSuccessCreatesReportContainingExpectedSuccessResultAndRemovesProduct)
{
    auto& fileSystemMock = setupFileSystemAndGetMock();
    auto products = defaultProducts();
    products[0].setProductHasChanged(false);
    products[1].setProductHasChanged(false);

    EXPECT_CALL(*mockSdds3Repo_, tryConnect(_, _, _)).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primary")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primary")).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, distribute());
    EXPECT_CALL(*mockSdds3Repo_, purge());
    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    EXPECT_CALL(*mockSdds3Repo_, getProducts()).Times(2).WillRepeatedly(Return(products));
    EXPECT_CALL(*mockSdds3Repo_, getSourceURL());
    EXPECT_CALL(*mockSdds3Repo_, getProductDistributionPath(isProduct("productRemove1")))
        .WillOnce(Return("/opt/sophos-spl/base/update/cache/sdds3primary/productRemove1"));

    EXPECT_CALL(*mockSdds3Repo_, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(*mockSdds3Repo_, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));
    TestSdds3RepositoryHelper::replaceSdds3RepositoryCreator([this]() { return std::move(mockSdds3Repo_); });

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
        toString(RepositoryStatus::SUCCESS));

    std::vector<std::string> fileListOfProductsToRemove = { "productRemove1.sh" };
    std::string uninstallPath = "/opt/sophos-spl/base/update/var/installedproducts";
    EXPECT_CALL(fileSystemMock, isDirectory(uninstallPath)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, listFiles(uninstallPath)).WillOnce(Return(fileListOfProductsToRemove));

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([]() {
      auto mockProcess = std::make_unique<StrictMock<MockProcess>>();
      EXPECT_CALL(*mockProcess, exec(HasSubstr("productRemove1"), _, _)).Times(1);
      EXPECT_CALL(*mockProcess, output()).WillOnce(Return("uninstalling product"));
      EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
      return mockProcess;
    });

    Common::ProcessImpl::ArgcAndEnv args("SulDownloader", { "/dir/input.json", "/dir/output.json" }, {});
    EXPECT_EQ(SulDownloader::main_entry(3, args.argc()), 0);
}

TEST_F(
    SULDownloaderSdds3Test,
    main_entry_onSuccessCreatesReportContainingExpectedUninstallFailedResult)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(1, 1, 0);

    auto products = defaultProducts();
    products[0].setProductHasChanged(false);
    products[1].setProductHasChanged(false);

    EXPECT_CALL(*mockSdds3Repo_, tryConnect(_, _, _)).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primary")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primary")).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, distribute());
    EXPECT_CALL(*mockSdds3Repo_, purge());
    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    EXPECT_CALL(*mockSdds3Repo_, getProducts()).Times(2).WillRepeatedly(Return(products));
    EXPECT_CALL(*mockSdds3Repo_, getSourceURL());
    EXPECT_CALL(*mockSdds3Repo_, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(*mockSdds3Repo_, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));
    EXPECT_CALL(*mockSdds3Repo_, getProductDistributionPath(isProduct("productRemove1"))).WillOnce(Return("productRemove1"));
    TestSdds3RepositoryHelper::replaceSdds3RepositoryCreator([this]() { return std::move(mockSdds3Repo_); });

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
        toString(RepositoryStatus::UNINSTALLFAILED), 0);

    std::vector<std::string> fileListOfProductsToRemove = { "productRemove1.sh" };
    std::string uninstallPath = "/opt/sophos-spl/base/update/var/installedproducts";
    EXPECT_CALL(fileSystemMock, isDirectory(uninstallPath)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, listFiles(uninstallPath)).WillOnce(Return(fileListOfProductsToRemove));

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([]() {
      auto mockProcess = std::make_unique<StrictMock<MockProcess>>();
      EXPECT_CALL(*mockProcess, exec(HasSubstr("productRemove1"), _, _)).Times(1);
      EXPECT_CALL(*mockProcess, output()).WillOnce(Return("uninstalling product"));
      EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(1));
      return mockProcess;
    });

    Common::ProcessImpl::ArgcAndEnv args("SulDownloader", { "/dir/input.json", "/dir/output.json" }, {});

    EXPECT_EQ(
        SulDownloader::main_entry(3, args.argc()), RepositoryStatus::UNINSTALLFAILED);
}

// the other execution paths were covered in main_entry_* tests.
TEST_F(
    SULDownloaderSdds3Test,
    fileEntriesAndRunDownloaderThrowIfCannotCreateOutputFile)
{
    auto filesystemMock = std::make_unique<NaggyMock<MockFileSystem>>();
    EXPECT_CALL(*filesystemMock, readFile("/dir/input.json")).WillRepeatedly(Return(jsonSettings(defaultSettings())));
    EXPECT_CALL(*filesystemMock, isDirectory("/dir/path/that/cannot/be/created/output.json")).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, isDirectory("/dir/path/that/cannot/be/created")).WillOnce(Return(false));
    m_replacer.replace(std::move(filesystemMock));

    EXPECT_THROW(
        SulDownloader::fileEntriesAndRunDownloader("/dir/input.json", "/dir/path/that/cannot/be/created/output.json", ""),
        SulDownloaderException);
}

// configAndRunDownloader
TEST_F(
    SULDownloaderSdds3Test,
    configAndRunDownloaderInvalidSettingsReportError_RepositoryStatus_UNSPECIFIED)
{
    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, readFile(_)).Times(10).WillRepeatedly(Return(""));
    m_replacer.replace(std::move(mockFileSystem));

    std::string reportContent;
    int exitCode = 0;
    auto settings = defaultSettings();
    settings.clear_sophoscdnurls(); // no sophos urls, the system can not connect to warehouses
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
            toString(RepositoryStatus::SUCCESS))));
    EXPECT_THAT(
        reportContent,
        ::testing::HasSubstr(toString(
            RepositoryStatus::UNSPECIFIED)));
}

TEST_F(SULDownloaderSdds3Test, configAndRunDownloader_IfSuccessfulAndNotSupplementOnlyThenWritesToFeaturesFile)
{
    // Expect that features are written
    const int expectedInstalledFeatures = 1;
    auto& fileSystemMock = setupFileSystemAndGetMock(1, 1, expectedInstalledFeatures);

    ON_CALL(fileSystemMock, readFile("inputFilePath")).WillByDefault(Return(jsonSettings(defaultSettings())));

    // Produce a successful report by mocking that products were already installed
    setupFileVersionCalls(fileSystemMock, "PRODUCT_VERSION = 1.2", "PRODUCT_VERSION = 1.2");

    auto products = defaultProducts();
    TimeTracker timeTracker;
    DownloadReport downloadReport =
        DownloadReport::Report("", products, {}, {}, &timeTracker, DownloadReport::VerifyState::VerifyCorrect);
    std::string previousJsonReport = DownloadReport::fromReport(downloadReport);
    ON_CALL(*mockSdds3Repo_, getProducts).WillByDefault(Return(products));
    TestSdds3RepositoryHelper::replaceSdds3RepositoryCreator([this]() { return std::move(mockSdds3Repo_); });

    // Not supplement-only
    bool supplementOnly = false;
    const auto [exitCode, reportContent, baseDowngraded] = SulDownloader::configAndRunDownloader(
        "inputFilePath", "previousInputFilePath", previousJsonReport, supplementOnly);

    // Exit code should be 0
    EXPECT_EQ(exitCode, 0);
}

TEST_F(SULDownloaderSdds3Test, configAndRunDownloader_IfSuccessfulAndSupplementOnlyThenDoesNotWriteToFeaturesFile)
{
    auto mockFilePermissions = std::make_unique<MockFilePermissions>();
    Tests::ScopedReplaceFilePermissions scopedReplaceFilePermissions(std::move(mockFilePermissions));

    // Expect features to never be written
    const int expectedInstalledFeatures = 0;
    auto& fileSystemMock = setupFileSystemAndGetMock(1, 1, expectedInstalledFeatures);

    ON_CALL(fileSystemMock, readFile("inputFilePath")).WillByDefault(Return(jsonSettings(defaultSettings())));

    // Produce a successful report by mocking that products were already installed
    setupFileVersionCalls(fileSystemMock, "PRODUCT_VERSION = 1.2", "PRODUCT_VERSION = 1.2");

    auto products = defaultProducts();
    TimeTracker timeTracker;
    DownloadReport downloadReport =
        DownloadReport::Report("", products, {}, {}, &timeTracker, DownloadReport::VerifyState::VerifyCorrect);
    std::string previousJsonReport = DownloadReport::fromReport(downloadReport);
    ON_CALL(*mockSdds3Repo_, getProducts).WillByDefault(Return(products));
    TestSdds3RepositoryHelper::replaceSdds3RepositoryCreator([this]() { return std::move(mockSdds3Repo_); });
    // Supplement-only
    bool supplementOnly = true;
    const auto [exitCode, reportContent, baseDowngraded] = SulDownloader::configAndRunDownloader(
        "inputFilePath", "previousInputFilePath", previousJsonReport, supplementOnly);

    // Exit code must be 0 otherwise the features would not be written anyway
    EXPECT_EQ(exitCode, 0);
}

// runSULDownloader
TEST_F(
    SULDownloaderSdds3Test,
    runSULDownloader_RepositoryConnectionFailureShouldCreateValidConnectionFailureReport)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(1,1, 0);
    RepositoryError wError;
    wError.Description = "Error description";
    wError.status = RepositoryStatus::CONNECTIONERROR;
    std::string statusError = toString(wError.status);
    DownloadedProductVector emptyProducts;

    EXPECT_CALL(*mockSdds3Repo_, hasError()).WillOnce(Return(true)).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, hasImmediateFailError()).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, getError()).WillRepeatedly(Return(wError));
    EXPECT_CALL(*mockSdds3Repo_, tryConnect(_, _, _)).WillRepeatedly(Return(false)); // failed tryConnect call
    EXPECT_CALL(*mockSdds3Repo_, getProducts()).WillOnce(Return(emptyProducts));
    EXPECT_CALL(*mockSdds3Repo_, listInstalledSubscriptions()).WillOnce(Return(subscriptionsFromProduct(emptyProducts)));

    EXPECT_CALL(*mockSdds3Repo_, getSourceURL());
    EXPECT_CALL(fileSystemMock, exists("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, readLines("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(defaultOverrideSettings()));
    TestSdds3RepositoryHelper::replaceSdds3RepositoryCreator([this]() { return std::move(mockSdds3Repo_); });
    Sdds3SimplifiedDownloadReport expectedDownloadReport{ wError.status, wError.Description, {}, false, {} };

    auto configurationData = configData(defaultSettings());
    Common::Policy::UpdateSettings previousConfigurationData;
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
TEST_F(
    SULDownloaderSdds3Test,
    runSULDownloader_RepositoryConnectionFailureShouldCreateValidConnectionFailureReportContainingPreviousReportDFata)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(1,1, 0);

    RepositoryError wError;
    wError.Description = "Error description";
    wError.status = RepositoryStatus::CONNECTIONERROR;
    std::string statusError = toString(wError.status);
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

    EXPECT_CALL(*mockSdds3Repo_, hasError()).WillOnce(Return(true)).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, hasImmediateFailError()).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, getError()).WillRepeatedly(Return(wError));
    EXPECT_CALL(*mockSdds3Repo_, tryConnect(_, _, _)).WillRepeatedly(Return(false)); // failed tryConnect call
    EXPECT_CALL(*mockSdds3Repo_, getProducts()).WillOnce(Return(emptyProducts));
    EXPECT_CALL(*mockSdds3Repo_, listInstalledSubscriptions()).WillOnce(Return(subscriptionsFromProduct(emptyProducts)));

    EXPECT_CALL(*mockSdds3Repo_, getSourceURL());
    EXPECT_CALL(fileSystemMock, exists("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, readLines("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(defaultOverrideSettings()));
    TestSdds3RepositoryHelper::replaceSdds3RepositoryCreator([this]() { return std::move(mockSdds3Repo_); });
    Sdds3SimplifiedDownloadReport expectedDownloadReport{ wError.status, wError.Description, productReports, false, {} };

    auto configurationData = configData(defaultSettings());
    Common::Policy::UpdateSettings previousConfigurationData;
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
TEST_F(
    SULDownloaderSdds3Test,
    runSULDownloader_supplement_only_RepositoryConnectionFailureShouldCreateValidConnectionFailureReport)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(1,1, 0);

    RepositoryError wError;
    wError.Description = "Error description";
    wError.status = RepositoryStatus::CONNECTIONERROR;
    std::string statusError = toString(wError.status);
    DownloadedProductVector emptyProducts;

    EXPECT_CALL(*mockSdds3Repo_, hasError()).WillOnce(Return(true)).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, hasImmediateFailError()).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockSdds3Repo_, getError()).WillRepeatedly(Return(wError));
    EXPECT_CALL(*mockSdds3Repo_, tryConnect(_, true, _)).WillRepeatedly(Return(false)); // failed tryConnect call
    EXPECT_CALL(*mockSdds3Repo_, getProducts()).WillOnce(Return(emptyProducts));
    EXPECT_CALL(*mockSdds3Repo_, listInstalledSubscriptions()).WillOnce(Return(subscriptionsFromProduct(emptyProducts)));

    EXPECT_CALL(*mockSdds3Repo_, getSourceURL());
    EXPECT_CALL(fileSystemMock, exists("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, readLines("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(defaultOverrideSettings()));
    TestSdds3RepositoryHelper::replaceSdds3RepositoryCreator([this]() { return std::move(mockSdds3Repo_); });
    Sdds3SimplifiedDownloadReport expectedDownloadReport{ wError.status, wError.Description, {}, false, {} };

    auto configurationData = configData(defaultSettings());
    Common::Policy::UpdateSettings previousConfigurationData;
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

TEST_F(
    SULDownloaderSdds3Test,
    runSULDownloader_RepositorySynchronizationFailureShouldCreateValidSyncronizationFailureReport)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(1, 1, 0);
    RepositoryError wError;
    wError.Description = "Error description";
    wError.status = RepositoryStatus::PACKAGESOURCEMISSING;
    std::string statusError = toString(wError.status);
    DownloadedProductVector emptyProducts;

    EXPECT_CALL(*mockSdds3Repo_, hasError())
        .WillOnce(Return(false)) // connection
        .WillOnce(Return(true))  // synchronization
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, tryConnect(_, _, _)).WillOnce(Return(true)); // successful tryConnect call
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primary")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primary")).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, getError()).WillRepeatedly(Return(wError));
    EXPECT_CALL(*mockSdds3Repo_, getProducts()).WillOnce(Return(emptyProducts));
    EXPECT_CALL(*mockSdds3Repo_, listInstalledSubscriptions()).WillOnce(Return(subscriptionsFromProduct(emptyProducts)));

    EXPECT_CALL(*mockSdds3Repo_, getSourceURL());
    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/installedproductversions/ServerProtectionLinux-Plugin-EDR.ini"))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, exists("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, readLines("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(defaultOverrideSettings()));
    TestSdds3RepositoryHelper::replaceSdds3RepositoryCreator([this]() { return std::move(mockSdds3Repo_); });

    Sdds3SimplifiedDownloadReport expectedDownloadReport{ wError.status, wError.Description, {}, false, {} };

    auto configurationData = configData(defaultSettings());
    Common::Policy::UpdateSettings previousConfigurationData;
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
TEST_F(SULDownloaderSdds3Test, runSULDownloader_onDistributeFailure)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(1, 1, 0);
    RepositoryError wError;
    wError.Description = "Error description";
    wError.status = RepositoryStatus::DOWNLOADFAILED;
    std::string statusError = toString(wError.status);
    DownloadedProductVector products = defaultProducts();

    RepositoryError productError;
    productError.Description = "Product Error description";
    productError.status = RepositoryStatus::DOWNLOADFAILED;

    products[0].setError(productError);
    products[1].setError(productError);

    ProductReportVector productReports = defaultProductReports();
    productReports[0].errorDescription = productError.Description;
    productReports[0].productStatus = ProductReport::ProductStatus::SyncFailed;

    productReports[1].errorDescription = productError.Description;
    productReports[1].productStatus = ProductReport::ProductStatus::SyncFailed;

    EXPECT_CALL(*mockSdds3Repo_, hasError())
        .WillOnce(Return(false)) // connection
        .WillOnce(Return(false)) // synchronization
        .WillOnce(Return(true))  // distribute
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, tryConnect(_, _, _)).WillOnce(Return(true)); // successful tryConnect call
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primary")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primary")).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, distribute());
    EXPECT_CALL(*mockSdds3Repo_, setWillInstall(_)).Times(2);

    EXPECT_CALL(*mockSdds3Repo_, getError()).WillRepeatedly(Return(wError));
    EXPECT_CALL(*mockSdds3Repo_, getProducts()).WillRepeatedly(Return(products));
    EXPECT_CALL(*mockSdds3Repo_, listInstalledSubscriptions()).WillOnce(Return(subscriptionsFromProduct(products)));

    EXPECT_CALL(*mockSdds3Repo_, getSourceURL());
    EXPECT_CALL(fileSystemMock, exists("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, readLines("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(defaultOverrideSettings()));
    TestSdds3RepositoryHelper::replaceSdds3RepositoryCreator([this]() { return std::move(mockSdds3Repo_); });

    Sdds3SimplifiedDownloadReport expectedDownloadReport{ wError.status, wError.Description, productReports, false, {} };

    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/VERSION.ini"))
        .WillRepeatedly(Return(false));
    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/installedproductversions/ServerProtectionLinux-Plugin-EDR.ini"))
        .WillRepeatedly(Return(false));

    auto configurationData = configData(defaultSettings());
    Common::Policy::UpdateSettings previousConfigurationData;
    configurationData.verifySettingsAreValid();
    DownloadReport previousDownloadReport = DownloadReport::Report("Not assigned");

    EXPECT_PRED_FORMAT2(
        downloadReportSimilar,
        expectedDownloadReport,
        SulDownloader::runSULDownloader(configurationData, previousConfigurationData, previousDownloadReport));
}

TEST_F(
    SULDownloaderSdds3Test,
    runSULDownloader_RepositorySynchronizationResultingInNoUpdateNeededShouldCreateValidSuccessReport)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(1, 1, 0);
    DownloadedProductVector products = defaultProducts();
    ProductReportVector productReports = defaultProductReports();

    for (auto& product : products)
    {
        product.setProductHasChanged(false);
    }

    EXPECT_CALL(*mockSdds3Repo_, hasError()).WillRepeatedly(Return(false));

    EXPECT_CALL(*mockSdds3Repo_, tryConnect(_, _, _)).WillOnce(Return(true)); // successful tryConnect call
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primary")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primary")).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, distribute());
    EXPECT_CALL(*mockSdds3Repo_, purge());
    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    EXPECT_CALL(*mockSdds3Repo_, getProducts()).Times(2).WillRepeatedly(Return(products));
    EXPECT_CALL(*mockSdds3Repo_, getSourceURL());
    EXPECT_CALL(*mockSdds3Repo_, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(*mockSdds3Repo_, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));
    std::vector<std::string> emptyFileList;
    std::string uninstallPath = "/opt/sophos-spl/base/update/var/installedproducts";
    EXPECT_CALL(fileSystemMock, isDirectory(uninstallPath)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, listFiles(uninstallPath)).WillOnce(Return(emptyFileList));
    EXPECT_CALL(fileSystemMock, exists("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, readLines("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(defaultOverrideSettings()));
    TestSdds3RepositoryHelper::replaceSdds3RepositoryCreator([this]() { return std::move(mockSdds3Repo_); });

    setupFileVersionCalls(fileSystemMock, "PRODUCT_VERSION = 1.1.3.703", "PRODUCT_VERSION = 1.1.3.703");

    Sdds3SimplifiedDownloadReport expectedDownloadReport{ RepositoryStatus::SUCCESS,
                                                     "",
                                                     productReports,
                                                     true,
                                                     productsInfo({ products[0], products[1] }) };

    auto configurationData = configData(defaultSettings());
    Common::Policy::UpdateSettings previousConfigurationData;
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
TEST_F(
    SULDownloaderSdds3Test,
    runSULDownloader_UpdateFailForInvalidSignature)
{
    MockFileSystem& fileSystemMock = setupFileSystemAndGetMock(1, 1, 0);
    auto configurationData = configData(defaultSettings());
    Common::Policy::UpdateSettings previousConfigurationData;
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


    VersigFactory::instance().replaceCreator([&counter]() {
      auto versig = std::make_unique<StrictMock<MockVersig>>();
      if (counter++ == 0)
      {
          EXPECT_CALL(*versig, verify(_, "/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component"))
              .WillOnce(Return(IVersig::VerifySignature::SIGNATURE_VERIFIED));
      }
      else
      {
          EXPECT_CALL(*versig, verify(_, "/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR"))
              .WillOnce(Return(IVersig::VerifySignature::SIGNATURE_FAILED));
      }

      return versig;
    });

    ProductReportVector productReports = defaultProductReports();
    productReports[0].productStatus = ProductReport::ProductStatus::VerifyFailed;
    productReports[1].productStatus = ProductReport::ProductStatus::VerifyFailed;

    // if up to distribution passed, warehouse never returns error = true
    EXPECT_CALL(*mockSdds3Repo_, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockSdds3Repo_, tryConnect(_, _, _)).WillOnce(Return(true)); // successful tryConnect call
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primary")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primary")).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, distribute());
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    EXPECT_CALL(*mockSdds3Repo_, setWillInstall(_)).Times(2);
    EXPECT_CALL(*mockSdds3Repo_, getProducts()).WillRepeatedly(Return(products));
    EXPECT_CALL(*mockSdds3Repo_, listInstalledSubscriptions()).WillOnce(Return(subscriptionsFromProduct(products)));
    EXPECT_CALL(*mockSdds3Repo_, getSourceURL());
    TestSdds3RepositoryHelper::replaceSdds3RepositoryCreator([this]() { return std::move(mockSdds3Repo_); });
    Sdds3SimplifiedDownloadReport expectedDownloadReport{
        RepositoryStatus::INSTALLFAILED, "Update failed", productReports, false, {}
    };

    expectedDownloadReport.Products[1].errorDescription = "Product ServerProtectionLinux-Plugin-EDR failed signature verification";
    DownloadReport previousDownloadReport = DownloadReport::Report("Not assigned");
    auto result = SulDownloader::runSULDownloader(configurationData, previousConfigurationData, previousDownloadReport);
    EXPECT_PRED_FORMAT2(downloadReportSimilar, expectedDownloadReport, result);
}

/**
 * Simulate error in installing base successfully but fail to install plugin
 */
TEST_F(
    SULDownloaderSdds3Test,
    runSULDownloader_PluginInstallationFailureShouldResultInValidInstalledFailedReport)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(1, 1, 0);

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
    EXPECT_CALL(fileSystemMock, getSystemCommandExecutablePath(_)).WillRepeatedly(Return("systemctl"));

    setupFileVersionCalls(fileSystemMock, "PRODUCT_VERSION = 1.1.3.0", "PRODUCT_VERSION = 1.1.3.703");

    int counter = 0;


    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&counter,this, &fileSystemMock]() {
        if (counter == 0 || counter == 3)
        {
            counter++;
            return mockSystemctlStatus();
        }
        else if (counter == 1)
        {
            counter++;
            return mockBaseInstall(fileSystemMock);
        }
        else if (counter == 2)
        {
            counter++;
            return mockEDRInstall(fileSystemMock, 5);
        }
        else
        {
            counter++;
            return mockSystemctlStart();
        }
   });

    ProductReportVector productReports = defaultProductReports();
    productReports[1].errorDescription = "Product ServerProtectionLinux-Plugin-EDR failed to install";

    // base upgraded, plugin failed.
    productReports[0].productStatus = ProductReport::ProductStatus::Upgraded;
    productReports[1].productStatus = ProductReport::ProductStatus::InstallFailed;

    // if up to distribution passed, warehouse never returns error = true
    EXPECT_CALL(*mockSdds3Repo_, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockSdds3Repo_, tryConnect(_, _, _)).WillOnce(Return(true)); // successful tryConnect call
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primary")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primary")).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, distribute());
    EXPECT_CALL(*mockSdds3Repo_, purge());
    EXPECT_CALL(*mockSdds3Repo_, setWillInstall(_)).Times(2);
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    EXPECT_CALL(*mockSdds3Repo_, getProducts()).WillRepeatedly(Return(products));
    EXPECT_CALL(*mockSdds3Repo_, getSourceURL());
    EXPECT_CALL(*mockSdds3Repo_, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(*mockSdds3Repo_, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));
    TestSdds3RepositoryHelper::replaceSdds3RepositoryCreator([this]() { return std::move(mockSdds3Repo_); });

    Sdds3SimplifiedDownloadReport expectedDownloadReport{ RepositoryStatus::INSTALLFAILED,
                                                     "Update failed",
                                                     productReports,
                                                     false,
                                                     productsInfo({ products[0], products[1] }) };

    auto configurationData = configData(defaultSettings());
    Common::Policy::UpdateSettings previousConfigurationData;
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
TEST_F(
    SULDownloaderSdds3Test,
    runSULDownloader_SuccessfulFullUpdateShouldResultInValidSuccessReport)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(1, 1, 0);
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
    EXPECT_CALL(fileSystemMock, getSystemCommandExecutablePath(_)).WillRepeatedly(Return("systemctl"));

    setupFileVersionCalls(fileSystemMock, "PRODUCT_VERSION = 1.1.3.0", "PRODUCT_VERSION = 1.1.3.703");


    int counter = 0;
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&counter,this, &fileSystemMock]() {
        if (counter == 0 || counter == 3)
        {
           counter++;
           return mockSystemctlStatus();
        }
        else if (counter == 1)
        {
           counter++;
           return mockBaseInstall(fileSystemMock);
        }
        else if (counter == 2)
        {
           counter++;
           return mockEDRInstall(fileSystemMock);
        }
        else
        {
           counter++;
           return mockSystemctlStart();
        }
    });

    ProductReportVector productReports = defaultProductReports();
    productReports[0].productStatus = ProductReport::ProductStatus::Upgraded;
    productReports[1].productStatus = ProductReport::ProductStatus::Upgraded;

    // if up to distribution passed, warehouse never returns error = true
    EXPECT_CALL(*mockSdds3Repo_, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockSdds3Repo_, tryConnect(_, _, _)).WillOnce(Return(true)); // successful tryConnect call
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primary")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primary")).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, distribute());
    EXPECT_CALL(*mockSdds3Repo_, purge());
    EXPECT_CALL(*mockSdds3Repo_, setWillInstall(_)).Times(2);
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    EXPECT_CALL(*mockSdds3Repo_, getProducts()).Times(2).WillRepeatedly(Return(products));
    EXPECT_CALL(*mockSdds3Repo_, getSourceURL());
    EXPECT_CALL(*mockSdds3Repo_, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(*mockSdds3Repo_, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));
    TestSdds3RepositoryHelper::replaceSdds3RepositoryCreator([this]() { return std::move(mockSdds3Repo_); });
    Sdds3SimplifiedDownloadReport expectedDownloadReport{ RepositoryStatus::SUCCESS,
                                                     "",
                                                     productReports,
                                                     true,
                                                     productsInfo({ products[0], products[1] }) };

    auto configurationData = configData(defaultSettings());
    Common::Policy::UpdateSettings previousConfigurationData;
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

TEST_F(
    SULDownloaderSdds3Test,
    runSULDownloader_SuccessfulUpdateToNewVersionShouldNotRunUninstallScriptsAndShouldResultInValidSuccessReport)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(1, 1, 0);
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
    EXPECT_CALL(fileSystemMock, getSystemCommandExecutablePath(_)).WillRepeatedly(Return("systemctl"));

    setupFileVersionCalls(fileSystemMock, "PRODUCT_VERSION = 1.1.3.703", "PRODUCT_VERSION = 1.1.3.703");

    int counter = 0;

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&counter,&fileSystemMock,this]() {

        if (counter == 0 || counter == 3)
        {
            counter++;
            return mockSystemctlStatus();
        }
        else if (counter == 1)
        {
             counter++;
             return mockBaseInstall(fileSystemMock);
        }
         else if (counter == 2)
        {
             counter++;
             return mockEDRInstall(fileSystemMock);
        }
        else
        {
         counter++;
         return mockSystemctlStart();
        }
    });

    ProductReportVector productReports = defaultProductReports();
    productReports[0].productStatus = ProductReport::ProductStatus::Upgraded;
    productReports[1].productStatus = ProductReport::ProductStatus::Upgraded;

    // if up to distribution passed, warehouse never returns error = true
    EXPECT_CALL(*mockSdds3Repo_, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockSdds3Repo_, tryConnect(_, _, _)).WillOnce(Return(true)); // successful tryConnect call
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primary")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primary")).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, distribute());
    EXPECT_CALL(*mockSdds3Repo_, purge());
    EXPECT_CALL(*mockSdds3Repo_, setWillInstall(_)).Times(2);
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    EXPECT_CALL(*mockSdds3Repo_, getProducts()).Times(2).WillRepeatedly(Return(products));
    EXPECT_CALL(*mockSdds3Repo_, getSourceURL());
    EXPECT_CALL(*mockSdds3Repo_, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(*mockSdds3Repo_, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));
    TestSdds3RepositoryHelper::replaceSdds3RepositoryCreator([this]() { return std::move(mockSdds3Repo_); });

    Sdds3SimplifiedDownloadReport expectedDownloadReport{ RepositoryStatus::SUCCESS,
                                                     "",
                                                     productReports,
                                                     true,
                                                     productsInfo({ products[0], products[1] }) };

    auto configurationData = configData(defaultSettings());
    Common::Policy::UpdateSettings previousConfigurationData;
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
TEST_F(
    SULDownloaderSdds3Test,
    runSULDownloader_SuccessfulUpdateToNewVersionShouldCheckPluginIsRunning)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(1, 1, 0);
    DownloadedProductVector products = defaultProducts();

    for (auto& product : products)
    {
        product.setProductHasChanged(true);
    }
    std::vector<std::string> installProducts = {"edr,ServerProtectionLinux-Plugin-EDR"};
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
    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/installedComponentTracker")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, readLines("/opt/sophos-spl/base/update/var/installedComponentTracker")).WillRepeatedly(Return(installProducts));
    EXPECT_CALL(fileSystemMock, getSystemCommandExecutablePath(_)).WillRepeatedly(Return("systemctl"));

    setupFileVersionCalls(fileSystemMock, "PRODUCT_VERSION = 1.1.3.703", "PRODUCT_VERSION = 1.1.3.703");

    int counter = 0;

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&counter,&fileSystemMock,this]() {

       if (counter == 1)
       {
           counter++;
           return mockBaseInstall(fileSystemMock);
       }
       else if (counter == 2)
       {
           counter++;
           return mockEDRInstall(fileSystemMock);
       }
       else if (counter == 0 || counter == 3)
       {
           counter++;
           return mockSystemctlStatus();
       }
       else if (counter == 4)
       {
           counter++;
           return mockSystemctlStart();
       }
       else
       {
           counter++;
           return mockIsEdrRunning();
       }
   });

    ProductReportVector productReports = defaultProductReports();
    productReports[0].productStatus = ProductReport::ProductStatus::Upgraded;
    productReports[1].productStatus = ProductReport::ProductStatus::Upgraded;

    // if up to distribution passed, warehouse never returns error = true
    EXPECT_CALL(*mockSdds3Repo_, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockSdds3Repo_, tryConnect(_, _, _)).WillOnce(Return(true)); // successful tryConnect call
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primary")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primary")).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, distribute());
    EXPECT_CALL(*mockSdds3Repo_, purge());
    EXPECT_CALL(*mockSdds3Repo_, setWillInstall(_)).Times(2);
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    EXPECT_CALL(*mockSdds3Repo_, getProducts()).Times(2).WillRepeatedly(Return(products));
    EXPECT_CALL(*mockSdds3Repo_, getSourceURL());
    EXPECT_CALL(*mockSdds3Repo_, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(*mockSdds3Repo_, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));
    TestSdds3RepositoryHelper::replaceSdds3RepositoryCreator([this]() { return std::move(mockSdds3Repo_); });

    Sdds3SimplifiedDownloadReport expectedDownloadReport{ RepositoryStatus::SUCCESS,
                                                          "",
                                                          productReports,
                                                          true,
                                                          productsInfo({ products[0], products[1] }) };

    auto configurationData = configData(defaultSettings());
    Common::Policy::UpdateSettings previousConfigurationData;
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

TEST_F(
    SULDownloaderSdds3Test,
    runSULDownloader_SuldownloaderWillStopProductIfItIsrunningBeforeUpdate)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(1, 1, 0);
    DownloadedProductVector products = defaultProducts();

    for (auto& product : products)
    {
        product.setProductHasChanged(true);
    }
    std::vector<std::string> installProducts = {"edr,ServerProtectionLinux-Plugin-EDR"};
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
    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/installedComponentTracker")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, readLines("/opt/sophos-spl/base/update/var/installedComponentTracker")).WillRepeatedly(Return(installProducts));
    EXPECT_CALL(fileSystemMock, getSystemCommandExecutablePath(_)).WillRepeatedly(Return("systemctl"));

    setupFileVersionCalls(fileSystemMock, "PRODUCT_VERSION = 1.1.3.703", "PRODUCT_VERSION = 1.1.3.703");

    int counter = 0;

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&counter,&fileSystemMock,this]() {

       if (counter == 2)
       {
           counter++;
           return mockBaseInstall(fileSystemMock);
       }
       if (counter == 1)
       {
           counter++;
           return mockSystemctlStop();
       }
       else if (counter == 3)
       {
           counter++;
           return mockEDRInstall(fileSystemMock);
       }
       else if (counter == 0)
       {
           counter++;
           return mockSystemctlStatus(0);
       }
       else if (counter == 4)
       {
           counter++;
           return mockSystemctlStatus();
       }
       else if (counter == 5)
       {
           counter++;
           return mockSystemctlStart();
       }
       else
       {
           counter++;
           return mockIsEdrRunning();
       }
    });

    ProductReportVector productReports = defaultProductReports();
    productReports[0].productStatus = ProductReport::ProductStatus::Upgraded;
    productReports[1].productStatus = ProductReport::ProductStatus::Upgraded;

    // if up to distribution passed, warehouse never returns error = true
    EXPECT_CALL(*mockSdds3Repo_, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockSdds3Repo_, tryConnect(_, _, _)).WillOnce(Return(true)); // successful tryConnect call
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primary")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primary")).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, distribute());
    EXPECT_CALL(*mockSdds3Repo_, purge());
    EXPECT_CALL(*mockSdds3Repo_, setWillInstall(_)).Times(2);
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    EXPECT_CALL(*mockSdds3Repo_, getProducts()).Times(2).WillRepeatedly(Return(products));
    EXPECT_CALL(*mockSdds3Repo_, getSourceURL());
    EXPECT_CALL(*mockSdds3Repo_, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(*mockSdds3Repo_, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));
    TestSdds3RepositoryHelper::replaceSdds3RepositoryCreator([this]() { return std::move(mockSdds3Repo_); });

    Sdds3SimplifiedDownloadReport expectedDownloadReport{ RepositoryStatus::SUCCESS,
                                                          "",
                                                          productReports,
                                                          true,
                                                          productsInfo({ products[0], products[1] }) };

    auto configurationData = configData(defaultSettings());
    Common::Policy::UpdateSettings previousConfigurationData;
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

TEST_F(
    SULDownloaderSdds3Test,
    runSULDownloader_SuccessfulUpdateToOlderVersionShouldRunUninstallScriptsAndShouldResultInValidSuccessReport)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(1, 1, 0);
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
    EXPECT_CALL(fileSystemMock, getSystemCommandExecutablePath(_)).WillRepeatedly(Return("systemctl"));


    setupFileVersionCalls(fileSystemMock, "PRODUCT_VERSION = 1.1.3.703", "PRODUCT_VERSION = 1.1.3.0");

    int counter = 0;

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&counter,&fileSystemMock,this]() {
       if (counter == 1 || counter == 4)
       {
           counter++;
           return mockSystemctlStatus();
       }
      else if (counter == 0)
      {
          counter++;
          return mockSimpleExec("bin/uninstall.sh");
      }
      else if (counter == 2)
      {
          counter++;

          return mockBaseInstall(fileSystemMock);
      }
      else if (counter == 3)
      {
          counter++;
          return mockEDRInstall(fileSystemMock);
      }
      else
      {
          counter++;
          return mockSystemctlStart();
      }
    });

    ProductReportVector productReports = defaultProductReports();
    productReports[0].productStatus = ProductReport::ProductStatus::Upgraded;
    productReports[1].productStatus = ProductReport::ProductStatus::Upgraded;

    // if up to distribution passed, warehouse never returns error = true
    EXPECT_CALL(*mockSdds3Repo_, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockSdds3Repo_, tryConnect(_, _, _)).WillOnce(Return(true)); // successful tryConnect call
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primary")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primary")).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, distribute());
    EXPECT_CALL(*mockSdds3Repo_, purge());
    EXPECT_CALL(*mockSdds3Repo_, setWillInstall(_)).Times(2);
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    EXPECT_CALL(*mockSdds3Repo_, getProducts()).Times(2).WillRepeatedly(Return(products));
    EXPECT_CALL(*mockSdds3Repo_, getSourceURL());
    EXPECT_CALL(*mockSdds3Repo_, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(*mockSdds3Repo_, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));
    TestSdds3RepositoryHelper::replaceSdds3RepositoryCreator([this]() { return std::move(mockSdds3Repo_); });

    Sdds3SimplifiedDownloadReport expectedDownloadReport{ RepositoryStatus::SUCCESS,
                                                     "",
                                                     productReports,
                                                     true,
                                                     productsInfo({ products[0], products[1] }) };

    auto configurationData = configData(defaultSettings());
    Common::Policy::UpdateSettings previousConfigurationData;
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

TEST_F(
    SULDownloaderSdds3Test,
    runSULDownloader_SuccessfulUpdateToOlderVersionOfPluginShouldOnlyRunPluginUninstallScriptsAndShouldResultInValidSuccessReport)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(1, 1, 0);
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
    EXPECT_CALL(fileSystemMock, getSystemCommandExecutablePath(_)).WillRepeatedly(Return("systemctl"));

    setupBaseVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 1.1.3.7", "PRODUCT_VERSION = 1.1.3.7");
    setupPluginVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 1.1.3.7", "PRODUCT_VERSION = 1.1.3.0");

    int counter = 0;

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&counter,&fileSystemMock,this]() {
      if (counter == 0)
      {
          counter++;
          return mockSimpleExec("ServerProtectionLinux-Plugin-EDR.sh");
      }
      else if (counter == 2)
      {
          counter++;
          return mockBaseInstall(fileSystemMock);
      }
      else if (counter == 3)
      {
          counter++;
          return mockEDRInstall(fileSystemMock);
      }
      else if (counter == 1 || counter == 4)
      {
          counter++;
          return mockSystemctlStatus();
      }
      else
      {
          counter++;
          return mockSystemctlStart();
      }
    });

    ProductReportVector productReports = defaultProductReports();
    productReports[0].productStatus = ProductReport::ProductStatus::Upgraded;
    productReports[1].productStatus = ProductReport::ProductStatus::Upgraded;

    // if up to distribution passed, warehouse never returns error = true
    EXPECT_CALL(*mockSdds3Repo_, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockSdds3Repo_, tryConnect(_, _, _)).WillOnce(Return(true)); // successful tryConnect call
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primary")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primary")).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, distribute());
    EXPECT_CALL(*mockSdds3Repo_, purge());
    EXPECT_CALL(*mockSdds3Repo_, setWillInstall(_)).Times(2);
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    EXPECT_CALL(*mockSdds3Repo_, getProducts()).Times(2).WillRepeatedly(Return(products));
    EXPECT_CALL(*mockSdds3Repo_, getSourceURL());
    EXPECT_CALL(*mockSdds3Repo_, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(*mockSdds3Repo_, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));
    TestSdds3RepositoryHelper::replaceSdds3RepositoryCreator([this]() { return std::move(mockSdds3Repo_); });

    Sdds3SimplifiedDownloadReport expectedDownloadReport{ RepositoryStatus::SUCCESS,
                                                     "",
                                                     productReports,
                                                     true,
                                                     productsInfo({ products[0], products[1] }) };

    auto configurationData = configData(defaultSettings());
    Common::Policy::UpdateSettings previousConfigurationData;
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

TEST_F(
    SULDownloaderSdds3Test,
    runSULDownloader_SuccessfulFullUpdateWithSubscriptionsDifferentFromProductsShouldBeReportedCorrectly)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(1, 1, 0);
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
    EXPECT_CALL(fileSystemMock, getSystemCommandExecutablePath(_)).WillRepeatedly(Return("systemctl"));


    setupFileVersionCalls(fileSystemMock, "PRODUCT_VERSION = 1.1.3.703", "PRODUCT_VERSION = 1.1.3.703");

    int counter = 0;

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&counter,&fileSystemMock,this]() {
      if (counter == 1)
      {
          counter++;
          return mockBaseInstall(fileSystemMock);
      }
      else if (counter == 2)
      {
          counter++;
          return mockEDRInstall(fileSystemMock);
      }
      else if (counter == 0 || counter == 3)
      {
          counter++;
          return mockSystemctlStatus();
      }
      else
      {
          counter++;
          return mockSystemctlStart();
      }
    });

    ProductReportVector productReports = defaultProductReports();
    productReports[0].productStatus = ProductReport::ProductStatus::Upgraded;
    productReports[1].productStatus = ProductReport::ProductStatus::Upgraded;

    // if up to distribution passed, warehouse never returns error = true
    EXPECT_CALL(*mockSdds3Repo_, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockSdds3Repo_, tryConnect(_, _, _)).WillOnce(Return(true)); // successful tryConnect call
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primary")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primary")).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, distribute());
    EXPECT_CALL(*mockSdds3Repo_, purge());
    EXPECT_CALL(*mockSdds3Repo_, setWillInstall(_)).Times(2);
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    EXPECT_CALL(*mockSdds3Repo_, getProducts()).Times(2).WillRepeatedly(Return(products));
    EXPECT_CALL(*mockSdds3Repo_, getSourceURL());
    EXPECT_CALL(*mockSdds3Repo_, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(*mockSdds3Repo_, listInstalledSubscriptions)
        .WillOnce(Return(subscriptionsInfo({ products[0] }))); // only product 0 in the subscriptions
    TestSdds3RepositoryHelper::replaceSdds3RepositoryCreator([this]() { return std::move(mockSdds3Repo_); });

    Sdds3SimplifiedDownloadReport expectedDownloadReport{ RepositoryStatus::SUCCESS,
                                                     "",
                                                     productReports,
                                                     true,
                                                     productsInfo({ products[0], products[1] }) };

    auto copyProduct = expectedDownloadReport.Products[0];
    expectedDownloadReport.Products.clear();
    expectedDownloadReport.Products.push_back(copyProduct);

    auto configurationData = configData(defaultSettings());
    Common::Policy::UpdateSettings previousConfigurationData;
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

TEST_F(
    SULDownloaderSdds3Test,
    runSULDownloader_ListOfSubscriptionListSizeDifferenceResultsInFullSuccessfulUpdate)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(2, 1, 0);
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
    EXPECT_CALL(fileSystemMock, getSystemCommandExecutablePath(_)).WillRepeatedly(Return("systemctl"));

    setupFileVersionCalls(fileSystemMock, "PRODUCT_VERSION = 1.1.3.703", "PRODUCT_VERSION = 1.1.3.703");

    int counter = 0;

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&counter,&fileSystemMock,this]() {
      if (counter == 1)
      {
          counter++;
          return mockBaseInstall(fileSystemMock);
      }
      else if (counter == 2)
      {
          counter++;
          return mockEDRInstall(fileSystemMock);
      }
      else if (counter == 0 || counter == 3)
      {
          counter++;
          return mockSystemctlStatus();
      }
      else
      {
          counter++;
          return mockSystemctlStart();
      }
    });

    ProductReportVector productReports = defaultProductReports();
    productReports[0].productStatus = ProductReport::ProductStatus::Upgraded;
    productReports[1].productStatus = ProductReport::ProductStatus::Upgraded;

    // if up to distribution passed, warehouse never returns error = true
    EXPECT_CALL(*mockSdds3Repo_, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockSdds3Repo_, tryConnect(_, _, _)).WillOnce(Return(true)); // successful tryConnect call
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primary")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primary")).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, distribute());
    EXPECT_CALL(*mockSdds3Repo_, purge());
    EXPECT_CALL(*mockSdds3Repo_, setWillInstall(_)).Times(1);
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    EXPECT_CALL(*mockSdds3Repo_, getProducts()).WillRepeatedly(Return(products));
    EXPECT_CALL(*mockSdds3Repo_, getSourceURL());
    EXPECT_CALL(*mockSdds3Repo_, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(*mockSdds3Repo_, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));
    TestSdds3RepositoryHelper::replaceSdds3RepositoryCreator([this]() { return std::move(mockSdds3Repo_); });

    Sdds3SimplifiedDownloadReport expectedDownloadReport{ RepositoryStatus::SUCCESS,
                                                     "",
                                                     productReports,
                                                     true,
                                                     productsInfo({ products[0], products[1] }) };

    auto previousConfigurationData = configData(defaultSettings());
    auto configurationData = configData(newSubscriptionSettings());

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

TEST_F(
    SULDownloaderSdds3Test,
    runSULDownloader_ListOfFeatureListSizeDifferenceResultsInFullSuccessfulUpdate)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(2, 1, 0);
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
    EXPECT_CALL(fileSystemMock, getSystemCommandExecutablePath(_)).WillRepeatedly(Return("systemctl"));

    setupFileVersionCalls(fileSystemMock, "PRODUCT_VERSION = 1.1.3.703", "PRODUCT_VERSION = 1.1.3.703");

    int counter = 0;

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&counter,&fileSystemMock,this]() {
      if (counter == 1)
      {
          counter++;
          return mockBaseInstall(fileSystemMock);
      }
      else if (counter == 2)
      {
          counter++;
          return mockEDRInstall(fileSystemMock);
      }
      else if (counter == 0 || counter == 3)
      {
          counter++;
          return mockSystemctlStatus();
      }
      else
      {
          counter++;
          return mockSystemctlStart();
      }
    });

    ProductReportVector productReports = defaultProductReports();
    productReports[0].productStatus = ProductReport::ProductStatus::Upgraded;
    productReports[1].productStatus = ProductReport::ProductStatus::Upgraded;

    // if up to distribution passed, warehouse never returns error = true
    EXPECT_CALL(*mockSdds3Repo_, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockSdds3Repo_, tryConnect(_, _, _)).WillOnce(Return(true)); // successful tryConnect call
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primary")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primary")).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, distribute());
    EXPECT_CALL(*mockSdds3Repo_, purge());
    EXPECT_CALL(*mockSdds3Repo_, setWillInstall(_)).Times(1);
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    EXPECT_CALL(*mockSdds3Repo_, getProducts()).WillOnce(Return(products));
    EXPECT_CALL(*mockSdds3Repo_, getSourceURL());
    EXPECT_CALL(*mockSdds3Repo_, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(*mockSdds3Repo_, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));
    TestSdds3RepositoryHelper::replaceSdds3RepositoryCreator([this]() { return std::move(mockSdds3Repo_); });

    Sdds3SimplifiedDownloadReport expectedDownloadReport{ RepositoryStatus::SUCCESS,
                                                     "",
                                                     productReports,
                                                     true,
                                                     productsInfo({ products[0], products[1] }) };

    auto previousConfigurationData = configData(defaultSettings());
    auto configurationData = configData(newFeatureSettings());

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

TEST_F(
    SULDownloaderSdds3Test,
    runSULDownloader_ListOfSubscriptionsWhichDontMatchPreviousConfigResultsInFullSuccessfulUpdate)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(2, 1, 0);
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
    EXPECT_CALL(fileSystemMock, getSystemCommandExecutablePath(_)).WillRepeatedly(Return("systemctl"));

    setupFileVersionCalls(fileSystemMock, "PRODUCT_VERSION = 1.1.3.703", "PRODUCT_VERSION = 1.1.3.703");

    int counter = 0;

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&counter,&fileSystemMock,this]() {
      if (counter == 1)
      {
          counter++;
          return mockBaseInstall(fileSystemMock);
      }
      else if (counter == 2)
      {
          counter++;
          return mockEDRInstall(fileSystemMock);
      }
      else if (counter == 0 || counter == 3)
      {
          counter++;
          return mockSystemctlStatus();
      }
      else
      {
          counter++;
          return mockSystemctlStart();
      }
    });

    ProductReportVector productReports = defaultProductReports();
    productReports[0].productStatus = ProductReport::ProductStatus::Upgraded;
    productReports[1].productStatus = ProductReport::ProductStatus::Upgraded;

    // if up to distribution passed, warehouse never returns error = true
    EXPECT_CALL(*mockSdds3Repo_, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockSdds3Repo_, tryConnect(_, _, _)).WillOnce(Return(true)); // successful tryConnect call
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primary")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primary")).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, distribute());
    EXPECT_CALL(*mockSdds3Repo_, purge());
    EXPECT_CALL(*mockSdds3Repo_, setWillInstall(_)).Times(1);
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    EXPECT_CALL(*mockSdds3Repo_, getProducts()).WillOnce(Return(products));
    EXPECT_CALL(*mockSdds3Repo_, getSourceURL());
    EXPECT_CALL(*mockSdds3Repo_, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(*mockSdds3Repo_, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));
    TestSdds3RepositoryHelper::replaceSdds3RepositoryCreator([this]() { return std::move(mockSdds3Repo_); });

    Sdds3SimplifiedDownloadReport expectedDownloadReport{ RepositoryStatus::SUCCESS,
                                                     "",
                                                     productReports,
                                                     true,
                                                     productsInfo({ products[0], products[1] }) };

    auto previousConfigurationData = configData(newSubscriptionSettings());
    auto configurationData = configData(newSubscriptionSettings("Plugin_2"));

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

TEST_F(
    SULDownloaderSdds3Test,
    runSULDownloader_ListOfFeaturesWhichDontMatchPreviousConfigResultsInFullSuccessfulUpdate)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(2, 1, 0);
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
    EXPECT_CALL(fileSystemMock, getSystemCommandExecutablePath(_)).WillRepeatedly(Return("systemctl"));

    setupFileVersionCalls(fileSystemMock, "PRODUCT_VERSION = 1.1.3.703", "PRODUCT_VERSION = 1.1.3.703");

    int counter = 0;

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&counter,&fileSystemMock,this]() {
      if (counter == 1)
      {
          counter++;
          return mockBaseInstall(fileSystemMock);
      }
      else if (counter == 2)
      {
          counter++;
          return mockEDRInstall(fileSystemMock);
      }
      else if (counter == 0 || counter == 3)
      {
          counter++;
          return mockSystemctlStatus();
      }
      else
      {
          counter++;
          return mockSystemctlStart();
      }
    });

    ProductReportVector productReports = defaultProductReports();
    productReports[0].productStatus = ProductReport::ProductStatus::Upgraded;
    productReports[1].productStatus = ProductReport::ProductStatus::Upgraded;

    // if up to distribution passed, warehouse never returns error = true
    EXPECT_CALL(*mockSdds3Repo_, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockSdds3Repo_, tryConnect(_, _, _)).WillOnce(Return(true)); // successful tryConnect call
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primary")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primary")).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, distribute());
    EXPECT_CALL(*mockSdds3Repo_, purge());
    EXPECT_CALL(*mockSdds3Repo_, setWillInstall(_)).Times(1);
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    EXPECT_CALL(*mockSdds3Repo_, getProducts()).WillOnce(Return(products));
    EXPECT_CALL(*mockSdds3Repo_, getSourceURL());
    EXPECT_CALL(*mockSdds3Repo_, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(*mockSdds3Repo_, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));
    TestSdds3RepositoryHelper::replaceSdds3RepositoryCreator([this]() { return std::move(mockSdds3Repo_); });

    Sdds3SimplifiedDownloadReport expectedDownloadReport{ RepositoryStatus::SUCCESS,
                                                     "",
                                                     productReports,
                                                     true,
                                                     productsInfo({ products[0], products[1] }) };

    auto previousConfigurationData = configData(newFeatureSettings());
    auto configurationData = configData(newFeatureSettings("DIFF_FEATURE"));

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

TEST_F(
    SULDownloaderSdds3Test,
    runSULDownloader_previousProductChangesShouldResultInSuccessfulFullUpdateAndProduceValidSuccessReport)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(1, 1, 0);
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
    EXPECT_CALL(fileSystemMock, getSystemCommandExecutablePath(_)).WillRepeatedly(Return("systemctl"));


    setupFileVersionCalls(fileSystemMock, "PRODUCT_VERSION = 1.1.3.703", "PRODUCT_VERSION = 1.1.3.703");

    int counter = 0;

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&counter,&fileSystemMock,this]() {

        if (counter == 1)
      {
          counter++;
          return mockBaseInstall(fileSystemMock);
      }
      else if (counter == 2)
      {
          counter++;
          return mockEDRInstall(fileSystemMock);
      }
      else if (counter == 0 || counter == 3)
      {
          counter++;
          return mockSystemctlStatus();
      }
      else
      {
          counter++;
          return mockSystemctlStart();
      }
    });

    ProductReportVector productReports = defaultProductReports();
    productReports[0].productStatus = ProductReport::ProductStatus::Upgraded;
    productReports[1].productStatus = ProductReport::ProductStatus::Upgraded;

    // if up to distribution passed, warehouse never returns error = true
    EXPECT_CALL(*mockSdds3Repo_, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockSdds3Repo_, tryConnect(_, _, _)).WillOnce(Return(true)); // successful tryConnect call
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primary")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primary")).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, distribute());
    EXPECT_CALL(*mockSdds3Repo_, purge());
    EXPECT_CALL(*mockSdds3Repo_, setWillInstall(_)).Times(2);
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    EXPECT_CALL(*mockSdds3Repo_, getProducts()).Times(2).WillRepeatedly(Return(products));
    EXPECT_CALL(*mockSdds3Repo_, getSourceURL());
    EXPECT_CALL(*mockSdds3Repo_, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(*mockSdds3Repo_, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));
    TestSdds3RepositoryHelper::replaceSdds3RepositoryCreator([this]() { return std::move(mockSdds3Repo_); });

    Sdds3SimplifiedDownloadReport expectedDownloadReport{ RepositoryStatus::SUCCESS,
                                                     "",
                                                     productReports,
                                                     true,
                                                     productsInfo({ products[0], products[1] }) };

    auto configurationData = configData(defaultSettings());
    Common::Policy::UpdateSettings previousConfigurationData;
    configurationData.verifySettingsAreValid();
    previousConfigurationData.verifySettingsAreValid();
    DownloadReport previousDownloadReport = DownloadReport::Report("Not assigned");

    EXPECT_PRED_FORMAT2(
        downloadReportSimilar,
        expectedDownloadReport,
        SulDownloader::runSULDownloader(configurationData, previousConfigurationData, previousDownloadReport));
}

TEST_F(
    SULDownloaderSdds3Test,
    runSULDownloader_WithUpdateConfigDataMatchingWarehouseSynchronizationResultingInNoUpdateNeededShouldCreateValidSuccessReport)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(1, 1, 0);
    DownloadedProductVector products = defaultProducts();
    ProductReportVector productReports = defaultProductReports();

    for (auto& product : products)
    {
        product.setProductHasChanged(false);
    }

    EXPECT_CALL(*mockSdds3Repo_, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockSdds3Repo_, tryConnect(_, false, _)).WillOnce(Return(true)); // successful tryConnect call
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primary")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primary")).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, distribute());
    EXPECT_CALL(*mockSdds3Repo_, purge());
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    EXPECT_CALL(*mockSdds3Repo_, getProducts()).Times(2).WillRepeatedly(Return(products));
    EXPECT_CALL(*mockSdds3Repo_, getSourceURL());
    EXPECT_CALL(*mockSdds3Repo_, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(*mockSdds3Repo_, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));

    std::vector<std::string> emptyFileList;
    std::string uninstallPath = "/opt/sophos-spl/base/update/var/installedproducts";
    EXPECT_CALL(fileSystemMock, isDirectory(uninstallPath)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, listFiles(uninstallPath)).WillOnce(Return(emptyFileList));
    EXPECT_CALL(fileSystemMock, exists("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, readLines("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(defaultOverrideSettings()));
    TestSdds3RepositoryHelper::replaceSdds3RepositoryCreator([this]() { return std::move(mockSdds3Repo_); });

    setupFileVersionCalls(fileSystemMock, "PRODUCT_VERSION = 1.1.3.703", "PRODUCT_VERSION = 1.1.3.703");

    Sdds3SimplifiedDownloadReport expectedDownloadReport{ RepositoryStatus::SUCCESS,
                                                     "",
                                                     productReports,
                                                     true,
                                                     productsInfo({ products[0], products[1] }) };

    auto configurationData = configData(defaultSettings());
    auto previousConfigurationData = configData(defaultSettings());
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

TEST_F(
    SULDownloaderSdds3Test,
    runSULDownloader_supplement_only_WithUpdateConfigDataMatchingWarehouseSynchronizationResultingInNoUpdateNeededShouldCreateValidSuccessReport)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(1, 1, 0);
    DownloadedProductVector products = defaultProducts();
    ProductReportVector productReports = defaultProductReports();
    for (auto& product : products)
    {
        product.setProductHasChanged(false);
    }

    EXPECT_CALL(*mockSdds3Repo_, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockSdds3Repo_, tryConnect(_, true, _)).WillOnce(Return(true)); // successful tryConnect call
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primary")).RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primary")).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, distribute());
    EXPECT_CALL(*mockSdds3Repo_, purge());
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    EXPECT_CALL(*mockSdds3Repo_, getProducts()).Times(2).WillRepeatedly(Return(products));
    EXPECT_CALL(*mockSdds3Repo_, getSourceURL());
    EXPECT_CALL(*mockSdds3Repo_, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(*mockSdds3Repo_, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));

    std::vector<std::string> emptyFileList;
    std::string uninstallPath = "/opt/sophos-spl/base/update/var/installedproducts";
    EXPECT_CALL(fileSystemMock, isDirectory(uninstallPath)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, listFiles(uninstallPath)).WillOnce(Return(emptyFileList));
    EXPECT_CALL(fileSystemMock, exists("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, readLines("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(defaultOverrideSettings()));

    TestSdds3RepositoryHelper::replaceSdds3RepositoryCreator([this]() { return std::move(mockSdds3Repo_); });
    setupFileVersionCalls(fileSystemMock, "PRODUCT_VERSION = 1.1.3.703", "PRODUCT_VERSION = 1.1.3.703");

    Sdds3SimplifiedDownloadReport expectedDownloadReport{ RepositoryStatus::SUCCESS,
                                                     "",
                                                     productReports,
                                                     true,
                                                     productsInfo({ products[0], products[1] }) };

    auto configurationData = configData(defaultSettings());
    auto previousConfigurationData = configData(defaultSettings());
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

TEST_F(
    SULDownloaderSdds3Test,
    runSULDownloader_checkLogVerbosityVERBOSE)
{
    auto& fileSystem = setupFileSystemAndGetMock(1, 1, 0);
    EXPECT_CALL(fileSystem, isFile("/etc/ssl/certs/ca-certificates.crt")).WillOnce(Return(false));
    EXPECT_CALL(fileSystem, isFile("/etc/pki/tls/certs/ca-bundle.crt")).WillOnce(Return(false));
    EXPECT_CALL(fileSystem, isFile("/etc/ssl/ca-bundle.pem")).WillOnce(Return(false));
    testing::internal::CaptureStderr();
    auto settings = defaultSettings();
    settings.clear_sophoscdnurls();
    settings.add_sophoscdnurls("http://localhost/latest/donotexits");
    settings.set_loglevel(ConfigurationSettings::VERBOSE);
    auto configurationData = configData(settings);
    Common::Policy::UpdateSettings previousConfigurationData;
    configurationData.verifySettingsAreValid();
    DownloadReport previousDownloadReport = DownloadReport::Report("Not assigned");
    EXPECT_CALL(fileSystem, exists("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystem, isFile("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystem, readLines("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini")).WillRepeatedly(Return(defaultOverrideSettings()));


    auto downloadReport =
        SulDownloader::runSULDownloader(configurationData, previousConfigurationData, previousDownloadReport);
    std::string errStd = testing::internal::GetCapturedStderr();
    ASSERT_THAT(errStd, ::testing::HasSubstr("Failed to connect to repository"));
}

TEST_F(
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
    settings.clear_sophoscdnurls();
    settings.add_sophoscdnurls("http://localhost/latest/donotexits");
    settings.set_loglevel(ConfigurationSettings::NORMAL);
    auto configurationData = configData(settings);
    Common::Policy::UpdateSettings previousConfigurationData;
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

TEST_F(
    SULDownloaderSdds3Test,
    runSULDownloader_failureToPurgeSDDS2CacheDoesNotHaltSync)
{
    auto& fileSystemMock = setupFileSystemAndGetMock();

    auto products = defaultProducts();
    products[0].setProductHasChanged(false);
    products[1].setProductHasChanged(false);

    EXPECT_CALL(*mockSdds3Repo_, tryConnect(_, _, _)).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(fileSystemMock, recursivelyDeleteContentsOfDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Throw(Common::FileSystem::IFileSystemException("error")));
    EXPECT_CALL(fileSystemMock, isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse")).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, synchronize(_,_,_)).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, distribute());
    EXPECT_CALL(*mockSdds3Repo_, purge());
    // the real warehouse will set DistributePath after distribute to the products
    products[0].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component");
    products[1].setDistributePath("/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Plugin-EDR");
    EXPECT_CALL(*mockSdds3Repo_, getProducts()).Times(2).WillRepeatedly(Return(products));
    EXPECT_CALL(*mockSdds3Repo_, getSourceURL());
    EXPECT_CALL(*mockSdds3Repo_, listInstalledProducts).WillOnce(Return(productsInfo({ products[0], products[1] })));
    EXPECT_CALL(*mockSdds3Repo_, listInstalledSubscriptions).WillOnce(Return(subscriptionsInfo({ products[0], products[1] })));

    TestSdds3RepositoryHelper::replaceSdds3RepositoryCreator([this]() { return std::move(mockSdds3Repo_); });
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
        toString(RepositoryStatus::SUCCESS));

    std::string uninstallPath = "/opt/sophos-spl/base/update/var/installedproducts";
    EXPECT_CALL(fileSystemMock, isDirectory(uninstallPath)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, listFiles(uninstallPath)).WillOnce(Return(emptyFileList));

    Common::ProcessImpl::ArgcAndEnv args("SulDownloader", { "/dir/input.json", "/dir/output.json" }, {});

    EXPECT_EQ(SulDownloader::main_entry(3, args.argc()), 0);
}

TEST_F(SULDownloaderSdds3Test,updateFailsIfNoJWTToken)
{
    setupFileSystemAndGetMock(1, 0, 0);
    testing::internal::CaptureStderr();

    auto settings = defaultSettings();
    settings.set_loglevel(ConfigurationSettings::NORMAL);
    settings.mutable_jwtoken()->assign("");
    auto configurationData = configData(settings);
    Common::Policy::UpdateSettings previousConfigurationData;
    configurationData.verifySettingsAreValid();
    DownloadReport previousDownloadReport = DownloadReport::Report("Not assigned");

    auto report =
        SulDownloader::runSULDownloader(configurationData, previousConfigurationData, previousDownloadReport);

    //We wont writeInstalledFeatures if return code is non zero
    EXPECT_EQ(report.getStatus(), suldownloaderdata::RepositoryStatus::DOWNLOADFAILED);

    std::string errStd = testing::internal::GetCapturedStderr();
    ASSERT_THAT(errStd, ::testing::HasSubstr("Failed to update because JWToken was empty"));
}

TEST_F(SULDownloaderSdds3Test,updateFailsIfOldVersion)
{
    setupFileSystemAndGetMock(1, 0, 0);
    testing::internal::CaptureStderr();

    auto settings = defaultSettings();
    auto primarySubscription = ProductSubscription("rigidname", "baseversion", "tag", "2020");
    settings.set_loglevel(ConfigurationSettings::NORMAL);
    auto configurationData = configData(settings);
    configurationData.setPrimarySubscription(primarySubscription);
    Common::Policy::UpdateSettings previousConfigurationData;
    configurationData.verifySettingsAreValid();
    DownloadReport previousDownloadReport = DownloadReport::Report("Not assigned");

    auto report =
        SulDownloader::runSULDownloader(configurationData, previousConfigurationData, previousDownloadReport);

    //We wont writeInstalledFeatures if return code is non zero
    EXPECT_EQ(report.getStatus(), suldownloaderdata::RepositoryStatus::DOWNLOADFAILED);

    std::string errStd = testing::internal::GetCapturedStderr();
    ASSERT_THAT(errStd, ::testing::HasSubstr("The requested fixed version is not available on SDDS3"));
}

TEST_F(SULDownloaderSdds3Test, runSULDownloader_NonSupplementOnlyClearsAwaitScheduledUpdateFlagAndTriesToInstall)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(0, 1, 0);

    // Expect an upgrade
    setupFileVersionCalls(fileSystemMock, "PRODUCT_VERSION = 1.2", "PRODUCT_VERSION = 1.3");

    auto settings = defaultSettings();
    auto configurationData = configData(settings);
    configurationData.verifySettingsAreValid();
    Common::Policy::UpdateSettings previousConfigurationData;
    const auto products = defaultProducts();
    TimeTracker timeTracker;
    DownloadReport previousDownloadReport =
        DownloadReport::Report("", products, {}, {},
                               &timeTracker, DownloadReport::VerifyState::VerifyCorrect);

    EXPECT_CALL(*mockSdds3Repo_, getProducts).WillRepeatedly(Return(products));
    EXPECT_CALL(*mockSdds3Repo_, hasError).WillRepeatedly(Return(false));
    TestSdds3RepositoryHelper::replaceSdds3RepositoryCreator([this]() { return std::move(mockSdds3Repo_); });

    EXPECT_CALL(
        fileSystemMock, removeFile("/opt/sophos-spl/base/update/var/updatescheduler/await_scheduled_update", _));
    EXPECT_CALL(fileSystemMock, getSystemCommandExecutablePath(_)).WillRepeatedly(Return("systemctl"));

    // Expect both installers to run
    int counter = 0;
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        [&counter,&fileSystemMock,this](){

            if (counter == 1)
            {
                counter++;
                return mockBaseInstall(fileSystemMock);
            }
            else if (counter == 2)
            {
                counter++;
                return mockEDRInstall(fileSystemMock);
            }
            else if (counter == 0 || counter == 3)
            {
                counter++;
                return mockSystemctlStatus();
            }
            else
            {
                counter++;
                return mockSystemctlStart();
            }
        });

    SulDownloader::runSULDownloader(configurationData, previousConfigurationData, previousDownloadReport, false);
}

TEST_F(SULDownloaderSdds3Test, runSULDownloader_SupplementOnlyBelowVersion123DoesNotClearAwaitScheduledUpdateFlagAndDoesNotInstall)
{
    testing::internal::CaptureStderr();

    auto& mockFileSystem = setupFileSystemAndGetMock(0, 1, 0);

    // Expect an upgrade
    setupFileVersionCalls(mockFileSystem, "PRODUCT_VERSION = 1.2.2.999", "PRODUCT_VERSION = 1.2.3.0");

    auto settings = defaultSettings();
    auto configurationData = configData(settings);
    configurationData.verifySettingsAreValid();
    Common::Policy::UpdateSettings previousConfigurationData;
    const auto products = defaultProducts();
    TimeTracker timeTracker;
    DownloadReport previousDownloadReport =
        DownloadReport::Report("", products, {}, {}, &timeTracker, DownloadReport::VerifyState::VerifyCorrect);

    ON_CALL(*mockSdds3Repo_, getProducts).WillByDefault(Return(products));
    TestSdds3RepositoryHelper::replaceSdds3RepositoryCreator([this]() { return std::move(mockSdds3Repo_); });

    EXPECT_CALL(mockFileSystem, removeFile("/opt/sophos-spl/base/update/var/updatescheduler/await_scheduled_update", _))
        .Times(0);

    // Expect no install process to be run
    int counter = 0;
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        [&counter]()
        {
            ++counter;
            return std::make_unique<MockProcess>();
        });

    SulDownloader::runSULDownloader(configurationData, previousConfigurationData, previousDownloadReport, true);

    EXPECT_EQ(counter, 0);

    std::string errStd = testing::internal::GetCapturedStderr();
    EXPECT_THAT(
        errStd,
        ::testing::HasSubstr("INFO Downloaded Product line: 'ServerProtectionLinux-Base-component' is up to date."));
    EXPECT_THAT(
        errStd,
        ::testing::HasSubstr("INFO Downloaded Product line: 'ServerProtectionLinux-Plugin-EDR' is up to date."));
}

TEST_F(
    SULDownloaderSdds3Test,
    runSULDownloader_SupplementOnlyButVersion123DoesNotClearAwaitScheduledUpdateFlagAndTriesToInstall)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(0, 1, 0);

    // Expect an upgrade, with the current installed version being >= 1.2.3
    setupFileVersionCalls(fileSystemMock, "PRODUCT_VERSION = 1.2.3.0", "PRODUCT_VERSION = 1.2.5.1");

    auto settings = defaultSettings();
    auto configurationData = configData(settings);
    configurationData.verifySettingsAreValid();
    Common::Policy::UpdateSettings previousConfigurationData;
    const auto products = defaultProducts();
    TimeTracker timeTracker;
    DownloadReport previousDownloadReport =
        DownloadReport::Report("", products, {}, {}, &timeTracker, DownloadReport::VerifyState::VerifyCorrect);

    ON_CALL(*mockSdds3Repo_, getProducts).WillByDefault(Return(products));
    TestSdds3RepositoryHelper::replaceSdds3RepositoryCreator([this]() { return std::move(mockSdds3Repo_); });

    EXPECT_CALL(
        fileSystemMock, removeFile("/opt/sophos-spl/base/update/var/updatescheduler/await_scheduled_update", _)).Times(0);

    // Expect both installers to run
    int counter = 0;
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        [&counter,&fileSystemMock,this]()
        {
            ++counter;
            if (counter == 1)
            {
                return mockBaseInstall(fileSystemMock);
            }
            else
            {
                return mockEDRInstall(fileSystemMock);
            }
        });

    const auto supplementOnly = true;
    SulDownloader::runSULDownloader(
        configurationData, previousConfigurationData, previousDownloadReport, supplementOnly);
}

TEST_F(SULDownloaderSdds3Test, RunSULDownloaderProductUpdateButBaseVersionIniDoesNotExistStillTriesToInstall)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(0, 1, 0);

    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/VERSION.ini")).WillRepeatedly(Return(false));

    // Expect a plugin upgrade
    setupPluginVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 1.2.2.999", "PRODUCT_VERSION = 1.2.3.0");

    auto settings = defaultSettings();
    auto configurationData = configData(settings);
    configurationData.verifySettingsAreValid();
    Common::Policy::UpdateSettings previousConfigurationData;
    const auto products = defaultProducts();
    TimeTracker timeTracker;
    DownloadReport previousDownloadReport =
        DownloadReport::Report("", products, {}, {}, &timeTracker, DownloadReport::VerifyState::VerifyCorrect);

    ON_CALL(*mockSdds3Repo_, getProducts).WillByDefault(Return(products));
    TestSdds3RepositoryHelper::replaceSdds3RepositoryCreator([this]() { return std::move(mockSdds3Repo_); });

    EXPECT_CALL(fileSystemMock, removeFile("/opt/sophos-spl/base/update/var/updatescheduler/await_scheduled_update", _))
        .Times(1);
    EXPECT_CALL(fileSystemMock, getSystemCommandExecutablePath(_)).WillRepeatedly(Return("systemctl"));

    // Expect both installers to run
    int counter = 0;
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
    [&counter,&fileSystemMock,this](){

        if (counter == 1)
        {
            counter++;
            return mockBaseInstall(fileSystemMock);
        }
        else if (counter == 2)
        {
            counter++;
            return mockEDRInstall(fileSystemMock);
        }
        else if (counter == 0 || counter == 3)
        {
            counter++;
            return mockSystemctlStatus();
        }
        else
        {
            counter++;
            return mockSystemctlStart();
        }
    });

    const auto supplementOnly = false;
    SulDownloader::runSULDownloader(
        configurationData, previousConfigurationData, previousDownloadReport, supplementOnly);
}

TEST_F(SULDownloaderSdds3Test, RunSULDownloaderSupplementOnlyButBaseVersionIniDoesNotExistDoesNotInstallAnything)
{
    auto& mockFileSystem = setupFileSystemAndGetMock(0, 1, 0);

     EXPECT_CALL(mockFileSystem, isFile("/opt/sophos-spl/base/VERSION.ini")).WillRepeatedly(Return(false));

    // Expect a plugin upgrade
    setupPluginVersionFileCalls(mockFileSystem, "PRODUCT_VERSION = 1.2.2.999", "PRODUCT_VERSION = 1.2.3.0");

    auto settings = defaultSettings();
    auto configurationData = configData(settings);
    configurationData.verifySettingsAreValid();
    Common::Policy::UpdateSettings previousConfigurationData;
    const auto products = defaultProducts();
    TimeTracker timeTracker;
    DownloadReport previousDownloadReport =
        DownloadReport::Report("", products, {}, {}, &timeTracker, DownloadReport::VerifyState::VerifyCorrect);

    ON_CALL(*mockSdds3Repo_, getProducts).WillByDefault(Return(products));
    TestSdds3RepositoryHelper::replaceSdds3RepositoryCreator([this]() { return std::move(mockSdds3Repo_); });

    EXPECT_CALL(mockFileSystem, removeFile("/opt/sophos-spl/base/update/var/updatescheduler/await_scheduled_update", _))
        .Times(0);

    // Expect no install process to be run
    int counter = 0;
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        [&counter]()
        {
            ++counter;
            return std::make_unique<MockProcess>();
        });

    SulDownloader::runSULDownloader(configurationData, previousConfigurationData, previousDownloadReport, true);

    EXPECT_EQ(counter, 0);
}

// Suldownloader WriteInstalledFeatures()
class TestSuldownloaderWriteInstalledFeaturesFunction: public LogOffInitializedTests{};

TEST_F(TestSuldownloaderWriteInstalledFeaturesFunction, featuresAreWrittenToJsonFile)
{
    setupInstalledFeaturesPermissions();
    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(
        *filesystemMock,
        writeFile(::testing::HasSubstr("installed_features.json"),"[\"feature1\",\"feature2\"]"));
    EXPECT_CALL(*filesystemMock,moveFile(_,
              Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath()));
    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{ std::move(filesystemMock) };
    std::vector<std::string> features = { "feature1", "feature2" };
    SulDownloader::writeInstalledFeatures(features);
}

TEST_F(TestSuldownloaderWriteInstalledFeaturesFunction, emptyListOfFeaturesAreWrittenToJsonFile)
{
    setupInstalledFeaturesPermissions();
    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*filesystemMock,writeFile(::testing::HasSubstr("installed_features.json"), "[]"));
    EXPECT_CALL(*filesystemMock,moveFile(_,
                 Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath()));
    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{ std::move(filesystemMock) };
    std::vector<std::string> features = {};
    SulDownloader::writeInstalledFeatures(features);
}

TEST_F(TestSuldownloaderWriteInstalledFeaturesFunction, noThrowExpectedOnFileSystemError)
{
    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(
        *filesystemMock,
        writeFile(::testing::HasSubstr("installed_features.json"),"[\"feature1\",\"feature2\"]"))
        .WillOnce(Throw(Common::FileSystem::IFileSystemException("error")));

    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{ std::move(filesystemMock) };

    std::vector<std::string> features = { "feature1", "feature2" };
    EXPECT_NO_THROW(SulDownloader::writeInstalledFeatures(features));
}

/*
 * Fault Injection
 */
std::string getUpdateConfig(const std::string& esmVersion)
{
    auto config = R"({
    "sophosCdnURLs": [
                       "http://ostia.eng.sophosinvalid/latest/Virt-vShieldInvalid"
    ],
    "sophosSusURL": "https://sus.sophosupd.com",
    "proxy": {
      "url": "noproxy:",
      "credential": {
          "username": "",
          "password": ""
      }
    },
    "JWToken": "token",
    "tenantId": "tenantid",
    "deviceId": "deviceid",
    "primarySubscription": {
      "rigidName": "ServerProtectionLinux-Base-component",
      "baseVersion": "",
      "tag": "RECOMMMENDED",
      "fixedVersion": ""
    },
    "features": [
                   "CORE"
    ],)"

    + esmVersion +

    R"(})";

    return config;
}

class TestSulDownloaderParameterizedInvalidESM
    :  public ::testing::TestWithParam<std::string>, public TestSulDownloaderSdds3Base
{
public:
    void SetUp() override
    {
        loggingSetup_ = Common::Logging::LOGOFFFORTEST();
    }

    void TearDown() override
    {
        Tests::restoreFileSystem();
        Test::TearDown();
    }

    Common::Logging::ConsoleLoggingSetup loggingSetup_;
};

INSTANTIATE_TEST_SUITE_P(
    SULDownloaderSdds3Test,
    TestSulDownloaderParameterizedInvalidESM,
    ::testing::Values(
        //One is missing
        R"("esmVersion": { "token": "token" })",
        R"("esmVersion": { "name": "name" })",
        //One is empty
        R"("esmVersion": { "token": "token", "name": "" })",
        R"("esmVersion": { "token": "", "name": "name" })"));


TEST_P(TestSulDownloaderParameterizedInvalidESM, invalidESMInputs)
{
    auto esmVersion = GetParam();
    auto updateConfig = getUpdateConfig(esmVersion);

    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, readFile(_)).WillOnce(Return(updateConfig));
    EXPECT_CALL(*mockFileSystem, isFile(_)).WillOnce(Return(false));
    m_replacer.replace(std::move(mockFileSystem));

    auto [exitCode, reportContent, baseDowngraded] =
        SulDownloader::configAndRunDownloader("inputFile", "", "");

    EXPECT_NE(exitCode, 0);
    EXPECT_TRUE(reportContent.find(toString(RepositoryStatus::SUCCESS)) == std::string::npos);
    EXPECT_FALSE(reportContent.find(toString(RepositoryStatus::UNSPECIFIED)) == std::string::npos);
}

class TestSulDownloaderParameterizedValidESM
    :  public ::testing::TestWithParam<std::string>, public TestSulDownloaderSdds3Base
{
public:
    void SetUp() override
    {
        loggingSetup_ = Common::Logging::LOGOFFFORTEST();

        VersigFactory::instance().replaceCreator([]() {
                                                     auto versig = std::make_unique<NiceMock<MockVersig>>();
                                                     ON_CALL(*versig, verify(_, _)).WillByDefault(Return(IVersig::VerifySignature::SIGNATURE_VERIFIED));
                                                     return versig;
                                                 });

        auto mockPidLockFileUtilsPtr = std::make_unique<NiceMock<MockPidLockFileUtils>>();
        ON_CALL(*mockPidLockFileUtilsPtr, write(_, _, _)).WillByDefault(Return(1));
        Common::FileSystemImpl::replacePidLockUtils(std::move(mockPidLockFileUtilsPtr));

        mockSdds3Repo_ = std::make_unique<NaggyMock<MockSdds3Repository>>();
    }

    /**
     * Remove the temporary directory.
     */
    void TearDown() override
    {
        Common::FileSystemImpl::restorePidLockUtils();
        VersigFactory::instance().restoreCreator();
        TestSdds3RepositoryHelper::restoreSdds3RepositoryFactory();
        Tests::restoreFilePermissions();
        Common::ApplicationConfiguration::restoreApplicationPathManager();
        Tests::restoreFileSystem();
        Test::TearDown();
    }

    std::unique_ptr<MockFileSystem> mockFileSystem_;
    Common::Logging::ConsoleLoggingSetup loggingSetup_;
};


INSTANTIATE_TEST_SUITE_P(
    SULDownloaderSdds3Test,
    TestSulDownloaderParameterizedValidESM,
    ::testing::Values(
        //Duplicate
        R"("esmVersion": { "token": "token", "token": "token2", "name": "name" })",
        //Both are empty
        R"("esmVersion": { "token": "", "name": "" })",
        //Both are missing
        R"("esmVersion": {})",
        //Large Strings
        R"("esmVersion": { "token": ")" + std::string(30000, 't') + R"(", "name": "name" })",
        R"("esmVersion": { "token": "token", "name": ")" + std::string(30000, 't') + R"(" })"));


TEST_P(TestSulDownloaderParameterizedValidESM, validESMInput)
{
    const std::string everest_installer = "/opt/sophos-spl/base/update/cache/sdds3primary/ServerProtectionLinux-Base-component/install.sh";
    auto esmVersion = GetParam();
    auto updateConfig = getUpdateConfig(esmVersion);

    auto products = defaultProducts();
    products[0].setProductHasChanged(false);
    products[0].setForceProductReinstall(false);
    products[0].setProductWillBeDowngraded(false);
    products.erase(std::next(products.begin()));

    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, readFile(_)).WillOnce(Return(updateConfig));
    EXPECT_CALL(*mockFileSystem, isFile(_)).Times(10).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockFileSystem, isFile("/opt/sophos-spl/base/VERSION.ini")).Times(5).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, readLines("/opt/sophos-spl/base/VERSION.ini"))
        .Times(4)
        .WillRepeatedly(Return(std::vector<std::string>{ "PRODUCT_VERSION = 1.1.3.703" }));
    EXPECT_CALL(*mockFileSystem, exists(_)).Times(3).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, isDirectory(_)).Times(3).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, isDirectory(everest_installer)).WillOnce(Return(false));
    EXPECT_CALL(
        *mockFileSystem,
        isDirectory(Common::ApplicationConfiguration::applicationPathManager().getLocalWarehouseRepository()))
        .WillOnce(Return(false));
    EXPECT_CALL(
        *mockFileSystem,
        isDirectory(Common::ApplicationConfiguration::applicationPathManager().getLocalDistributionRepository()))
        .WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, removeFile(_, _)).Times(5);
    EXPECT_CALL(*mockFileSystem, listFiles(_)).WillOnce(Return(std::vector<Path>{}));
    EXPECT_CALL(*mockFileSystem, currentWorkingDirectory()).Times(1);
    EXPECT_CALL(*mockFileSystem, writeFile(_, _)).Times(2);
    EXPECT_CALL(*mockFileSystem, makeExecutable(_)).Times(1);
    EXPECT_CALL(*mockFileSystem, getSystemCommandExecutablePath(_)).Times(3);
    m_replacer.replace(std::move(mockFileSystem));

    EXPECT_CALL(*mockSdds3Repo_, tryConnect(_, _, _)).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, hasError()).Times(3).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockSdds3Repo_, synchronize(_, _, _)).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, getProducts()).Times(2).WillRepeatedly(Return(products));
    EXPECT_CALL(*mockSdds3Repo_, distribute()).Times(1);
    EXPECT_CALL(*mockSdds3Repo_, getSourceURL()).Times(1);
    EXPECT_CALL(*mockSdds3Repo_, purge()).Times(1);
    EXPECT_CALL(*mockSdds3Repo_, setWillInstall(_)).Times(1);
    EXPECT_CALL(*mockSdds3Repo_, listInstalledSubscriptions()).WillOnce(Return(std::vector<suldownloaderdata::SubscriptionInfo> {}));
    EXPECT_CALL(*mockSdds3Repo_, listInstalledProducts).WillOnce(Return(productsInfo({ products[0] })));
    TestSdds3RepositoryHelper::replaceSdds3RepositoryCreator([&]() { return std::move(mockSdds3Repo_); });

    auto [exitCode, reportContent, baseDowngraded] =
        SulDownloader::configAndRunDownloader("inputFile", "", "");
    PRINT(reportContent);
    EXPECT_EQ(exitCode, 103);
    //We dont care the install failed, just that the esm duplicate didnt cause us to not request
    EXPECT_FALSE(reportContent.find(toString(RepositoryStatus::INSTALLFAILED)) == std::string::npos);
}

TEST_F(SULDownloaderSdds3Test, NonUTF8InConfig)
{
    UsingMemoryAppender memoryAppender(*this);

    auto esmVersion = generateNonUTF8String();
    auto updateConfig = getUpdateConfig(esmVersion);

    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, readFile(_)).WillOnce(Return(updateConfig));
    m_replacer.replace(std::move(mockFileSystem));

    auto [exitCode, reportContent, baseDowngraded] =
        SulDownloader::configAndRunDownloader("inputFile", "", "");

    EXPECT_NE(exitCode, 0);
    EXPECT_TRUE(reportContent.find(toString(RepositoryStatus::SUCCESS)) == std::string::npos);
    EXPECT_FALSE(reportContent.find(toString(RepositoryStatus::UNSPECIFIED)) == std::string::npos);
    EXPECT_TRUE(appenderContains("Not a valid UTF-8 string"));
}

TEST_F(SULDownloaderSdds3Test, xmlInConfig)
{
    UsingMemoryAppender memoryAppender(*this);

    std::string esmVersion(R"("esmVersion": <?xml version="1.0"?> <token>token</token>})");
    auto updateConfig = getUpdateConfig(esmVersion);

    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, readFile(_)).Times(10).WillRepeatedly(Return(updateConfig));
    m_replacer.replace(std::move(mockFileSystem));

    auto [exitCode, reportContent, baseDowngraded] =
        SulDownloader::configAndRunDownloader("inputFile", "", "");

    EXPECT_NE(exitCode, 0);
    EXPECT_TRUE(reportContent.find(toString(RepositoryStatus::SUCCESS)) == std::string::npos);
    EXPECT_FALSE(reportContent.find(toString(RepositoryStatus::UNSPECIFIED)) == std::string::npos);
    EXPECT_TRUE(appenderContains("Failed to process json message with error: INVALID_ARGUMENT:Expected a value.\n<?xml version=\"1.0\"?\n^"));
}