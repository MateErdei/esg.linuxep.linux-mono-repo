// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "MockSdds3Repository.h"
#include "MockSignatureVerifierWrapper.h"
#include "MockSusRequester.h"
#include "MockVersig.h"
#include "TestSdds3RepositoryHelper.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/DownloadReport/DownloadReport.h"
#include "Common/DownloadReport/RepositoryStatus.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/FileSystemImpl/PidLockFile.h"
#include "Common/Logging/ConsoleLoggingSetup.h"
#include "Common/Policy/ConfigurationSettings.pb.h"
#include "Common/ProcessImpl/ArgcAndEnv.h"
#include "Common/ProcessImpl/ProcessImpl.h"
#include "Common/ProtobufUtil/MessageUtility.h"
#include "Common/UtilityImpl/ProjectNames.h"
#include "SulDownloader/SulDownloader.h"
#include "SulDownloader/sdds3/ISusRequester.h"
#include "SulDownloader/sdds3/SDDS3Repository.h"
#include "SulDownloader/suldownloaderdata/ConfigurationData.h"
#include "SulDownloader/suldownloaderdata/DownloadReportBuilder.h"
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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
using namespace Common::Policy;
using namespace Common::DownloadReport;
using namespace SulDownloader::suldownloaderdata;
using DownloadedProductVector = std::vector<DownloadedProduct>;
using ProductReportVector = std::vector<ProductReport>;

using PolicyProto::ConfigurationSettings;

struct Sdds3SimplifiedDownloadReport {
  RepositoryStatus Status;
  std::string Description;
  std::vector<ProductReport> Products;
  std::vector<ProductInfo> WarehouseComponents;
};
namespace {
class TestSulDownloaderSdds3Base {
public:
  static std::vector<std::string> defaultOverrideSettings() {
    std::vector<std::string> lines = {"URLS = http://127.0.0.1:8080"};
    return lines;
  }
  static ConfigurationSettings defaultSettings() {
    ConfigurationSettings settings;

    settings.add_sophoscdnurls(
        "http://ostia.eng.sophosinvalid/latest/Virt-vShieldInvalid");
    settings.mutable_sophossusurl()->assign("https://sus.sophosupd.co");
    settings.mutable_proxy()->set_url("noproxy:");
    settings.mutable_proxy()->mutable_credential()->set_username("");
    settings.mutable_proxy()->mutable_credential()->set_password("");
    auto *proto_subscription = settings.mutable_primarysubscription();
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

  std::string getUpdateConfig(const std::string &esmVersion) {
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

  ConfigurationSettings
  newFeatureSettings(const std::string &feature = "SOME_FEATURE") {
    ConfigurationSettings settings = defaultSettings();
    settings.add_features(feature);
    return settings;
  }

  ConfigurationSettings
  newSubscriptionSettings(const std::string &rigidname = "Plugin_1") {
    ConfigurationSettings settings = defaultSettings();
    auto *proto_subscriptions = settings.mutable_products();

    PolicyProto::ConfigurationSettings_Subscription proto_subscription;

    proto_subscription.set_rigidname(rigidname);
    proto_subscription.set_baseversion("");
    proto_subscription.set_tag("RECOMMENDED");
    proto_subscription.set_fixedversion("");

    proto_subscriptions->Add(
        dynamic_cast<PolicyProto::ConfigurationSettings_Subscription &&>(
            proto_subscription));

    return settings;
  }

  std::string jsonSettings(const ConfigurationSettings &configSettings) {
    return Common::ProtobufUtil::MessageUtility::protoBuf2Json(configSettings);
  }

  UpdateSettings configData(const ConfigurationSettings &configSettings) {
    std::string json_output = jsonSettings(configSettings);
    return ConfigurationData::fromJsonSettings(json_output);
  }

  DownloadedProductVector defaultProducts() {
    DownloadedProductVector products;
    for (auto &metadata : defaultMetadata()) {
      DownloadedProduct product(metadata);
      product.setDistributePath(
          "/opt/sophos-spl/base/update/cache/sdds3primary");
      products.push_back(product);
    }
    products[0].setDistributePath(
        "/opt/sophos-spl/base/update/cache/sdds3primary/"
        "ServerProtectionLinux-Base-component");
    products[1].setDistributePath(
        "/opt/sophos-spl/base/update/cache/sdds3primary/"
        "ServerProtectionLinux-Plugin-EDR");
    return products;
  }

  std::vector<ProductInfo>
  productsInfo(const DownloadedProductVector &products) {
    std::vector<ProductInfo> info;
    for (auto &product : products) {
      ProductInfo pInfo;
      ProductMetadata metadata = product.getProductMetadata();
      pInfo.m_rigidName = metadata.getLine();
      pInfo.m_productName = metadata.getName();
      pInfo.m_version = metadata.getVersion();
      info.push_back(pInfo);
    }
    return info;
  }

  std::vector<suldownloaderdata::SubscriptionInfo>
  subscriptionsInfo(const DownloadedProductVector &products) {
    std::vector<suldownloaderdata::SubscriptionInfo> subInfos;
    for (auto &product : products) {
      SubscriptionInfo pInfo;
      ProductMetadata metadata = product.getProductMetadata();
      pInfo.rigidName = metadata.getLine();
      pInfo.version = metadata.getVersion();
      subInfos.push_back(pInfo);
    }
    return subInfos;
  }

  std::vector<ProductMetadata> defaultMetadata() {
    ProductMetadata base;
    base.setDefaultHomePath("ServerProtectionLinux-Base-component");
    base.setLine("ServerProtectionLinux-Base-component");
    base.setName("ServerProtectionLinux-Base-component-Product");
    base.setVersion("10.2.3");
    base.setTags({{"RECOMMENDED", "10", "Base-label"}});

    ProductMetadata plugin;
    plugin.setDefaultHomePath("ServerProtectionLinux-Plugin-EDR");
    plugin.setLine("ServerProtectionLinux-Plugin-EDR");
    plugin.setName("ServerProtectionLinux-Plugin-EDR-Product");
    plugin.setVersion("10.3.5");
    plugin.setTags({{"RECOMMENDED", "10", "Plugin-label"}});

    return std::vector<ProductMetadata>{base, plugin};
  }

  ProductReportVector defaultProductReports() {
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
    return ProductReportVector{base, plugin};
  }

  /**
   * @return BORROWED reference to MockFileSystem
   */
  MockFileSystem &
  setupFileSystemAndGetMock(int expectCallCount = 1, int expectCurrentProxy = 1,
                            int expectedInstalledFeatures = 1,
                            std::string installedFeatures = R"(["CORE"])") {
    Common::ApplicationConfiguration::applicationConfiguration().setData(
        Common::ApplicationConfiguration::SOPHOS_INSTALL, "/opt/sophos-spl");

    auto filesystemMock = std::make_unique<MockFileSystem>();
    EXPECT_CALL(*filesystemMock, isFile).Times(AnyNumber());
    EXPECT_CALL(*filesystemMock, isDirectory).Times(AnyNumber());
    EXPECT_CALL(*filesystemMock, removeFile(_, _)).Times(AnyNumber());
    if (expectCallCount > 0) {
      EXPECT_CALL(*filesystemMock, isDirectory("/opt/sophos-spl"))
          .Times(expectCallCount)
          .WillRepeatedly(Return(true));
      EXPECT_CALL(*filesystemMock,
                  isDirectory("/opt/sophos-spl/base/update/cache/"))
          .Times(expectCallCount)
          .WillRepeatedly(Return(true));
    } else {
      ON_CALL(*filesystemMock, isDirectory("/opt/sophos-spl"))
          .WillByDefault(Return(true));
      ON_CALL(*filesystemMock,
              isDirectory("/opt/sophos-spl/base/update/cache/"))
          .WillByDefault(Return(true));
    }

    EXPECT_CALL(*filesystemMock, exists(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, currentWorkingDirectory())
        .WillRepeatedly(Return("/opt/sophos-spl/base/bin"));
    EXPECT_CALL(*filesystemMock,
                isFile("/opt/sophos-spl/base/etc/savedproxy.config"))
        .WillRepeatedly(Return(false));
    std::string currentProxyFilePath =
        Common::ApplicationConfiguration::applicationPathManager()
            .getMcsCurrentProxyFilePath();
    if (expectCurrentProxy > 0) {
      EXPECT_CALL(*filesystemMock, isFile(currentProxyFilePath))
          .Times(expectCurrentProxy)
          .WillRepeatedly(Return(false));
    }
    std::string installedFeaturesFile =
        Common::ApplicationConfiguration::applicationPathManager()
            .getFeaturesJsonPath();
    EXPECT_CALL(*filesystemMock,
                writeFile(::testing::HasSubstr("installed_features.json"),
                          installedFeatures))
        .Times(expectedInstalledFeatures);

    setupExpectanceWriteProductUpdate(*filesystemMock);

    auto *pointer = filesystemMock.get();
    m_replacer.replace(std::move(filesystemMock));
    return *pointer;
  }

  void setupFileVersionCalls(MockFileSystem &fileSystemMock,
                             const std::string &currentVersion,
                             const std::string &newVersion) {
    setupBaseVersionFileCalls(fileSystemMock, currentVersion, newVersion);
    setupPluginVersionFileCalls(fileSystemMock, currentVersion, newVersion);
  }

  static void setupBaseVersionFileCalls(MockFileSystem &fileSystemMock,
                                        const std::string &currentVersion,
                                        const std::string &newVersion) {
    std::vector<std::string> currentVersionContents{{currentVersion}};
    std::vector<std::string> newVersionContents{{newVersion}};

    EXPECT_CALL(fileSystemMock,
                isFile("/opt/sophos-spl/base/update/cache/sdds3primary/"
                       "ServerProtectionLinux-Base-component/VERSION.ini"))
        .WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock,
                readLines("/opt/sophos-spl/base/update/cache/sdds3primary/"
                          "ServerProtectionLinux-Base-component/VERSION.ini"))
        .WillOnce(Return(newVersionContents));
    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/VERSION.ini"))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, readLines("/opt/sophos-spl/base/VERSION.ini"))
        .WillRepeatedly(Return(currentVersionContents));
  }

  void setupPluginVersionFileCalls(MockFileSystem &fileSystemMock,
                                   const std::string &currentVersion,
                                   const std::string &newVersion) {
    std::vector<std::string> currentVersionContents{{currentVersion}};
    std::vector<std::string> newVersionContents{{newVersion}};

    EXPECT_CALL(fileSystemMock,
                isFile("/opt/sophos-spl/base/update/cache/sdds3primary/"
                       "ServerProtectionLinux-Plugin-EDR/VERSION.ini"))
        .WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock,
                readLines("/opt/sophos-spl/base/update/cache/sdds3primary/"
                          "ServerProtectionLinux-Plugin-EDR/VERSION.ini"))
        .WillOnce(Return(newVersionContents));
    EXPECT_CALL(
        fileSystemMock,
        isFile("/opt/sophos-spl/base/update/var/installedproductversions/"
               "ServerProtectionLinux-Plugin-EDR.ini"))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(
        fileSystemMock,
        readLines("/opt/sophos-spl/base/update/var/installedproductversions/"
                  "ServerProtectionLinux-Plugin-EDR.ini"))
        .WillRepeatedly(Return(currentVersionContents));
  }

  void setupExpectanceWriteProductUpdate(MockFileSystem &mockFileSystem) {
    EXPECT_CALL(
        mockFileSystem,
        writeFile(
            "/opt/sophos-spl/var/suldownloader_last_product_update.marker", ""))
        .Times(::testing::AtMost(1));
    EXPECT_CALL(
        mockFileSystem,
        writeFile(Common::ApplicationConfiguration::applicationPathManager()
                      .getUpdateMarkerFile(),
                  "/opt/sophos-spl/base/bin"))
        .Times(::testing::AtMost(1));
    EXPECT_CALL(
        mockFileSystem,
        removeFile(Common::ApplicationConfiguration::applicationPathManager()
                       .getUpdateMarkerFile()))
        .Times(::testing::AtMost(1));
    EXPECT_CALL(
        mockFileSystem,
        isFile(Common::ApplicationConfiguration::applicationPathManager()
                   .getUpdateMarkerFile()))
        .Times(::testing::AtMost(1));
  }

  void setupExpectanceWriteAtomically(MockFileSystem &mockFileSystem,
                                      const std::string &contains,
                                      int installedFeaturesWritesExpected = 1) {
    EXPECT_CALL(mockFileSystem,
                writeFile(::testing::HasSubstr("/opt/sophos-spl/tmp"),
                          ::testing::HasSubstr(contains)));
    EXPECT_CALL(mockFileSystem, moveFile(_, "/dir/output.json"));
    EXPECT_CALL(
        mockFileSystem,
        moveFile(_, Common::ApplicationConfiguration::applicationPathManager()
                        .getFeaturesJsonPath()))
        .Times(installedFeaturesWritesExpected);

    auto mockFilePermissions =
        std::make_unique<StrictMock<MockFilePermissions>>();
    EXPECT_CALL(*mockFilePermissions,
                chown(testing::HasSubstr("/opt/sophos-spl/tmp"),
                      sophos::updateSchedulerUser(), "root"));
    mode_t expectedFilePermissions = S_IRUSR | S_IWUSR;
    EXPECT_CALL(*mockFilePermissions,
                chmod(testing::HasSubstr("/opt/sophos-spl/tmp"),
                      expectedFilePermissions));

    EXPECT_CALL(*mockFilePermissions,
                chown(testing::HasSubstr("installed_features.json"),
                      sophos::updateSchedulerUser(), sophos::group()))
        .Times(installedFeaturesWritesExpected);
    EXPECT_CALL(*mockFilePermissions,
                chmod(testing::HasSubstr("installed_features.json"),
                      S_IRUSR | S_IWUSR | S_IRGRP))
        .Times(installedFeaturesWritesExpected);

    EXPECT_CALL(*mockFilePermissions, getGroupId(sophos::group()))
        .WillRepeatedly(Return(123));
    EXPECT_CALL(*mockFilePermissions,
                chown(Common::ApplicationConfiguration::applicationPathManager()
                          .getSulDownloaderLockFilePath(),
                      "root", sophos::group()))
        .WillRepeatedly(Return());
    EXPECT_CALL(*mockFilePermissions,
                chmod(Common::ApplicationConfiguration::applicationPathManager()
                          .getSulDownloaderLockFilePath(),
                      S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP))
        .WillRepeatedly(Return());

    Tests::replaceFilePermissions(std::move(mockFilePermissions));
  }

  ::testing::AssertionResult
  downloadReportSimilar(const char *m_expr, const char *n_expr,
                        const Sdds3SimplifiedDownloadReport &expected,
                        const DownloadReport &resulted) {
    std::stringstream s;
    s << m_expr << " and " << n_expr << " failed: ";

    if (expected.Status != resulted.getStatus()) {
      return ::testing::AssertionFailure()
             << s.str()
             << " status differ: \n expected: " << toString(expected.Status)
             << "\n result: " << toString(resulted.getStatus());
    }
    if (expected.Description != resulted.getDescription()) {
      return ::testing::AssertionFailure()
             << s.str()
             << " Description differ: \n expected: " << expected.Description
             << "\n result: " << resulted.getDescription();
    }

    std::string expectedProductsSerialized =
        productsToString(expected.Products);
    std::string resultedProductsSerialized =
        productsToString(resulted.getProducts());
    if (expectedProductsSerialized != resultedProductsSerialized) {
      return ::testing::AssertionFailure()
             << s.str() << " Different products. \n expected: "
             << expectedProductsSerialized
             << "\n result: " << resultedProductsSerialized;
    }


    return listProductInfoIsEquivalent(m_expr, n_expr,
                                       expected.WarehouseComponents,
                                       resulted.getRepositoryComponents());
  }

  std::string productsToString(const ProductReportVector &productsReport) {
    std::stringstream stream;
    for (auto &productReport : productsReport) {
      stream << "name: " << productReport.name
             << ", version: " << productReport.downloadedVersion
             << ", error: " << productReport.errorDescription
             << ", productstatus: " << productReport.statusToString() << '\n';
    }
    std::string serialized = stream.str();
    if (serialized.empty()) {
      return "{empty}";
    }
    return serialized;
  }

  std::unique_ptr<Common::Process::IProcess>
  mockBaseInstall(MockFileSystem &mockFileSystem, int exitCode = 0) {
    auto mockProcess = std::make_unique<StrictMock<MockProcess>>();
    EXPECT_CALL(
        *mockProcess,
        exec(HasSubstr("ServerProtectionLinux-Base-component/install.sh"), _,
             _))
        .Times(1);
    EXPECT_CALL(*mockProcess, wait(_, _))
        .WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
    EXPECT_CALL(*mockProcess, output()).WillOnce(Return("installing base"));
    EXPECT_CALL(mockFileSystem, writeFile(_, "installing base"))
        .WillOnce(Return());
    EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(exitCode));
    return mockProcess;
  }

  std::unique_ptr<Common::Process::IProcess>
  mockEDRInstall(MockFileSystem &mockFileSystem, int exitCode = 0) {
    auto mockProcess = std::make_unique<StrictMock<MockProcess>>();
    EXPECT_CALL(
        *mockProcess,
        exec(HasSubstr("ServerProtectionLinux-Plugin-EDR/install.sh"), _, _))
        .Times(1);
    EXPECT_CALL(*mockProcess, wait(_, _))
        .WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
    EXPECT_CALL(*mockProcess, output()).WillOnce(Return("installing plugin"));
    EXPECT_CALL(mockFileSystem, writeFile(_, "installing plugin"))
        .WillOnce(Return());
    EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(exitCode));
    return mockProcess;
  }

  std::unique_ptr<Common::Process::IProcess>
  mockSystemctlStatus(int exitCode = 4) {
    auto mockProcess = std::make_unique<StrictMock<MockProcess>>();
    std::vector<std::string> stop_args = {"status", "sophos-spl"};
    EXPECT_CALL(*mockProcess, exec("systemctl", stop_args)).Times(1);
    EXPECT_CALL(*mockProcess, wait(_, _))
        .WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
    EXPECT_CALL(*mockProcess, output()).WillOnce(Return("watchdog running!"));
    EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(exitCode));
    return mockProcess;
  }

  std::unique_ptr<Common::Process::IProcess>
  mockSystemctlStop(int exitCode = 0) {
    auto mockProcess = std::make_unique<StrictMock<MockProcess>>();
    std::vector<std::string> start_args = {"stop", "sophos-spl"};
    EXPECT_CALL(*mockProcess, exec("systemctl", start_args)).Times(1);
    EXPECT_CALL(*mockProcess, wait(_, _))
        .WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
    EXPECT_CALL(*mockProcess, output()).WillOnce(Return(""));
    EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(exitCode));
    return mockProcess;
  }
  std::unique_ptr<Common::Process::IProcess>
  mockSystemctlStart(int exitCode = 0) {
    auto mockProcess = std::make_unique<StrictMock<MockProcess>>();
    std::vector<std::string> start_args = {"start", "sophos-spl"};
    EXPECT_CALL(*mockProcess, exec("systemctl", start_args)).Times(1);
    EXPECT_CALL(*mockProcess, wait(_, _))
        .WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
    EXPECT_CALL(*mockProcess, output()).WillOnce(Return(""));
    EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(exitCode));
    return mockProcess;
  }

  std::unique_ptr<Common::Process::IProcess>
  mockIsEdrRunning(int exitCode = 0) {
    auto mockProcess = std::make_unique<StrictMock<MockProcess>>();
    std::vector<std::string> start_args = {"isrunning", "edr"};
    EXPECT_CALL(*mockProcess, exec(HasSubstr("wdctl"), start_args)).Times(1);
    EXPECT_CALL(*mockProcess, wait(_, _))
        .WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
    EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(exitCode));
    return mockProcess;
  }

  std::unique_ptr<Common::Process::IProcess>
  mockSimpleExec(const std::string &substr, int exitCode = 0) {
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

} // namespace