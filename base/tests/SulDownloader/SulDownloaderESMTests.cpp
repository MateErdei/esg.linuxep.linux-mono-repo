// Copyright 2023 Sophos Limited. All rights reserved.

#include "SuldownloaderSDDS3Helper.h"

class TestSulDownloaderParameterizedInvalidESM
    : public ::testing::TestWithParam<std::string>,
      public TestSulDownloaderSdds3Base {
public:
  void SetUp() override { loggingSetup_ = Common::Logging::LOGOFFFORTEST(); }

  void TearDown() override {
    Tests::restoreFilePermissions();
    Tests::restoreFileSystem();
    Test::TearDown();
  }

  Common::Logging::ConsoleLoggingSetup loggingSetup_;
};

INSTANTIATE_TEST_SUITE_P(
    SULDownloaderSdds3Test, TestSulDownloaderParameterizedInvalidESM,
    ::testing::Values(
        // One is missing
        R"("esmVersion": { "token": "token" })",
        R"("esmVersion": { "name": "name" })",
        // One is empty
        R"("esmVersion": { "token": "token", "name": "" })",
        R"("esmVersion": { "token": "", "name": "name" })"));

TEST_P(TestSulDownloaderParameterizedInvalidESM, invalidESMInputs) {
  auto esmVersion = GetParam();
  auto updateConfig = getUpdateConfig(esmVersion);

  auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
  EXPECT_CALL(*mockFileSystem, readFile(_)).WillOnce(Return(updateConfig));
  EXPECT_CALL(*mockFileSystem, isFile(_)).WillOnce(Return(false));
  m_replacer.replace(std::move(mockFileSystem));

  auto [exitCode, reportContent, baseDowngraded] =
      SulDownloader::configAndRunDownloader("inputFile", "", "");

  EXPECT_NE(exitCode, 0);
  EXPECT_TRUE(reportContent.find(toString(RepositoryStatus::SUCCESS)) ==
              std::string::npos);
  EXPECT_FALSE(reportContent.find(toString(RepositoryStatus::UNSPECIFIED)) ==
               std::string::npos);
}

class TestSulDownloaderParameterizedValidESM
    : public ::testing::TestWithParam<std::string>,
      public TestSulDownloaderSdds3Base {
public:
  void SetUp() override {
    loggingSetup_ = Common::Logging::LOGOFFFORTEST();

    VersigFactory::instance().replaceCreator([]() {
      auto versig = std::make_unique<NiceMock<MockVersig>>();
      ON_CALL(*versig, verify(_, _))
          .WillByDefault(Return(IVersig::VerifySignature::SIGNATURE_VERIFIED));
      return versig;
    });

    auto mockPidLockFileUtilsPtr =
        std::make_unique<NiceMock<MockPidLockFileUtils>>();
    ON_CALL(*mockPidLockFileUtilsPtr, write(_, _, _)).WillByDefault(Return(1));
    Common::FileSystemImpl::replacePidLockUtils(
        std::move(mockPidLockFileUtilsPtr));

    mockSdds3Repo_ = std::make_unique<NaggyMock<MockSdds3Repository>>();
  }

  /**
   * Remove the temporary directory.
   */
  void TearDown() override {
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
    SULDownloaderSdds3Test, TestSulDownloaderParameterizedValidESM,
    ::testing::Values(
        // Duplicate
        R"("esmVersion": { "token": "token", "token": "token2", "name": "name" })",
        // Both are empty
        R"("esmVersion": { "token": "", "name": "" })",
        // Both are missing
        R"("esmVersion": {})",
        // Large Strings
        R"("esmVersion": { "token": ")" + std::string(30000, 't') +
            R"(", "name": "name" })",
        R"("esmVersion": { "token": "token", "name": ")" +
            std::string(30000, 't') + R"(" })"));

TEST_P(TestSulDownloaderParameterizedValidESM, validESMInput)
{
  const std::string everest_installer =
      "/opt/sophos-spl/base/update/cache/sdds3primary/"
      "ServerProtectionLinux-Base-component/install.sh";
  auto esmVersion = GetParam();
  auto updateConfig = getUpdateConfig(esmVersion);

  auto products = defaultProducts();
  products[0].setProductHasChanged(false);
  products[0].setForceProductReinstall(false);
  products[0].setProductWillBeDowngraded(false);
  products.erase(std::next(products.begin()));

  auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
  EXPECT_CALL(*mockFileSystem, readFile(_)).WillOnce(Return(updateConfig));
  EXPECT_CALL(*mockFileSystem, isFile(_))
      .Times(10)
      .WillRepeatedly(Return(false));
  EXPECT_CALL(*mockFileSystem, isFile("/opt/sophos-spl/base/VERSION.ini"))
      .Times(5)
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*mockFileSystem, readLines("/opt/sophos-spl/base/VERSION.ini"))
      .Times(4)
      .WillRepeatedly(
          Return(std::vector<std::string>{"PRODUCT_VERSION = 1.1.3.703"}));
  EXPECT_CALL(*mockFileSystem, exists(_)).Times(3).WillRepeatedly(Return(true));
  EXPECT_CALL(*mockFileSystem, isDirectory(_))
      .Times(3)
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*mockFileSystem, isDirectory(everest_installer))
      .WillOnce(Return(false));
  EXPECT_CALL(
      *mockFileSystem,
      isDirectory(Common::ApplicationConfiguration::applicationPathManager()
                      .getLocalWarehouseRepository()))
      .WillOnce(Return(false));
  EXPECT_CALL(
      *mockFileSystem,
      isDirectory(Common::ApplicationConfiguration::applicationPathManager()
                      .getLocalDistributionRepository()))
      .WillOnce(Return(false));
  EXPECT_CALL(*mockFileSystem, removeFile(_, _)).Times(5);
  EXPECT_CALL(*mockFileSystem, listFiles(_))
      .WillOnce(Return(std::vector<Path>{}));
  EXPECT_CALL(*mockFileSystem, currentWorkingDirectory()).Times(1);
  EXPECT_CALL(*mockFileSystem, writeFile(_, _)).Times(2);
  EXPECT_CALL(*mockFileSystem, makeExecutable(_)).Times(1);
  EXPECT_CALL(*mockFileSystem, getSystemCommandExecutablePath(_)).Times(3);
  m_replacer.replace(std::move(mockFileSystem));

  EXPECT_CALL(*mockSdds3Repo_, tryConnect(_, _, _)).WillOnce(Return(true));
  EXPECT_CALL(*mockSdds3Repo_, hasError())
      .Times(3)
      .WillRepeatedly(Return(false));
  EXPECT_CALL(*mockSdds3Repo_, synchronize(_, _, _)).WillOnce(Return(true));
  EXPECT_CALL(*mockSdds3Repo_, getProducts())
      .Times(2)
      .WillRepeatedly(Return(products));
  EXPECT_CALL(*mockSdds3Repo_, distribute()).Times(1);
  EXPECT_CALL(*mockSdds3Repo_, getSourceURL()).Times(1);
  EXPECT_CALL(*mockSdds3Repo_, purge()).Times(1);
  EXPECT_CALL(*mockSdds3Repo_, setWillInstall(_)).Times(1);
  EXPECT_CALL(*mockSdds3Repo_, listInstalledSubscriptions())
      .WillOnce(Return(std::vector<suldownloaderdata::SubscriptionInfo>{}));
  EXPECT_CALL(*mockSdds3Repo_, listInstalledProducts)
      .WillOnce(Return(productsInfo({products[0]})));
  TestSdds3RepositoryHelper::replaceSdds3RepositoryCreator(
      [&]() { return std::move(mockSdds3Repo_); });

  auto [exitCode, reportContent, baseDowngraded] =
      SulDownloader::configAndRunDownloader("inputFile", "", "");
  PRINT(reportContent);
  EXPECT_EQ(exitCode, 103);
  // We don't care the install failed, just that the esm duplicate didnt cause
  // us to not request
  EXPECT_FALSE(reportContent.find(toString(RepositoryStatus::INSTALLFAILED)) ==
               std::string::npos);
}
