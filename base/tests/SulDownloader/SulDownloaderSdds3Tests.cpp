// Copyright 2018-2023 Sophos Limited. All rights reserved.

/**
 * Component tests to SULDownloader mocking out Sdds3Repository
 */

#include "MockSdds3Repository.h"
#include "MockVersig.h"
#include "SuldownloaderSDDS3Helper.h"
#include "TestSdds3RepositoryHelper.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/DownloadReport/DownloadReport.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/FileSystemImpl/PidLockFile.h"
#include "Common/Logging/ConsoleLoggingSetup.h"
#include "Common/Policy/ConfigurationSettings.pb.h"
#include "Common/ProcessImpl/ArgcAndEnv.h"
#include "Common/ProcessImpl/ProcessImpl.h"
#include "Common/UtilityImpl/ProjectNames.h"
#include "SulDownloader/SulDownloader.h"
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
#include "tests/Common/UtilityImpl/TestStringGenerator.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace Common::Policy;
using namespace SulDownloader::suldownloaderdata;
using namespace Common::DownloadReport;

using PolicyProto::ConfigurationSettings;

namespace
{

    void setupInstalledFeaturesPermissions()
    {
        auto mockFilePermissions = std::make_unique<StrictMock<MockFilePermissions>>();
        EXPECT_CALL(*mockFilePermissions, chown(::testing::HasSubstr("installed_features.json"), sophos::updateSchedulerUser(), sophos::group()));
        EXPECT_CALL(*mockFilePermissions, chmod(::testing::HasSubstr("installed_features.json"), S_IRUSR | S_IWUSR | S_IRGRP));
        Tests::replaceFilePermissions(std::move(mockFilePermissions));
    }

} // namespace



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
    setupBaseVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.2.3");
    setupPluginVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.3.5");

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

    setupBaseVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.2.3");
    setupPluginVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.3.5");

    TimeTracker timeTracker;
    timeTracker.setStartTime(std::time_t(0));
    
    DownloadReport downloadReport =
        DownloadReportBuilder::Report("", products, {}, {}, &timeTracker, DownloadReportBuilder::VerifyState::VerifyCorrect);
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

    setupBaseVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.2.3");
    setupPluginVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.3.5");

    TimeTracker timeTracker;
    timeTracker.setStartTime(std::time_t(0));
    
    DownloadReport downloadReport =
        DownloadReportBuilder::Report("", products, {}, {}, &timeTracker, DownloadReportBuilder::VerifyState::VerifyCorrect);
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

    setupBaseVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.2.3");
    setupPluginVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.3.5");

    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/installedproductversions/productRemove1.ini")).WillOnce(Return(false));

    TimeTracker timeTracker;
    timeTracker.setStartTime(std::time_t(0));
    
    DownloadReport downloadReport =
        DownloadReportBuilder::Report("", products, {}, {}, &timeTracker, DownloadReportBuilder::VerifyState::VerifyCorrect);
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

    setupBaseVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.2.3");
    setupPluginVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.3.5");

    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/installedproductversions/productRemove1.ini")).WillOnce(Return(false));

    TimeTracker timeTracker;
    timeTracker.setStartTime(std::time_t(0));
    
    DownloadReport downloadReport =
        DownloadReportBuilder::Report("", products, {}, {}, &timeTracker, DownloadReportBuilder::VerifyState::VerifyCorrect);
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
    setupBaseVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.2.3");
    setupPluginVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.3.5");

    auto products = defaultProducts();
    TimeTracker timeTracker;
    DownloadReport downloadReport =
        DownloadReportBuilder::Report("", products, {}, {}, &timeTracker, DownloadReportBuilder::VerifyState::VerifyCorrect);
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
    setupBaseVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.2.3");
    setupPluginVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.3.5");

    auto products = defaultProducts();
    TimeTracker timeTracker;
    DownloadReport downloadReport =
        DownloadReportBuilder::Report("", products, {}, {}, &timeTracker, DownloadReportBuilder::VerifyState::VerifyCorrect);
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
    Sdds3SimplifiedDownloadReport expectedDownloadReport{ wError.status, wError.Description, {}, {} };

    auto configurationData = configData(defaultSettings());
    Common::Policy::UpdateSettings previousConfigurationData;
    configurationData.verifySettingsAreValid();
    DownloadReport previousDownloadReport = DownloadReportBuilder::Report("Not assigned");

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
    Sdds3SimplifiedDownloadReport expectedDownloadReport{ wError.status, wError.Description, productReports, {} };

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
    Sdds3SimplifiedDownloadReport expectedDownloadReport{ wError.status, wError.Description, {}, {} };

    auto configurationData = configData(defaultSettings());
    Common::Policy::UpdateSettings previousConfigurationData;
    configurationData.verifySettingsAreValid();
    DownloadReport previousDownloadReport = DownloadReportBuilder::Report("Not assigned");

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

    Sdds3SimplifiedDownloadReport expectedDownloadReport{ wError.status, wError.Description, {}, {} };

    auto configurationData = configData(defaultSettings());
    Common::Policy::UpdateSettings previousConfigurationData;
    configurationData.verifySettingsAreValid();
    DownloadReport previousDownloadReport = DownloadReportBuilder::Report("Not assigned");

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

    Sdds3SimplifiedDownloadReport expectedDownloadReport{ wError.status, wError.Description, productReports, {} };

    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/VERSION.ini"))
        .WillRepeatedly(Return(false));
    EXPECT_CALL(fileSystemMock, isFile("/opt/sophos-spl/base/update/var/installedproductversions/ServerProtectionLinux-Plugin-EDR.ini"))
        .WillRepeatedly(Return(false));

    auto configurationData = configData(defaultSettings());
    Common::Policy::UpdateSettings previousConfigurationData;
    configurationData.verifySettingsAreValid();
    DownloadReport previousDownloadReport = DownloadReportBuilder::Report("Not assigned");

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

    setupBaseVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.2.3");
    setupPluginVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.3.5");

    Sdds3SimplifiedDownloadReport expectedDownloadReport{ RepositoryStatus::SUCCESS,
                                                     "",
                                                     productReports,
                                                     productsInfo({ products[0], products[1] }) };

    auto configurationData = configData(defaultSettings());
    Common::Policy::UpdateSettings previousConfigurationData;
    configurationData.verifySettingsAreValid();

    TimeTracker timeTracker;
    timeTracker.setStartTime(std::time_t(0));
    
    DownloadReport previousDownloadReport =
        DownloadReportBuilder::Report("", products, {}, {}, &timeTracker, DownloadReportBuilder::VerifyState::VerifyCorrect);

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

    setupBaseVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.2.3");
    setupPluginVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.3.5");
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
        RepositoryStatus::INSTALLFAILED, "Update failed", productReports, {}
    };

    expectedDownloadReport.Products[1].errorDescription = "Product ServerProtectionLinux-Plugin-EDR failed signature verification";
    DownloadReport previousDownloadReport = DownloadReportBuilder::Report("Not assigned");
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

    setupBaseVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.2.2"); // Will upgrade to 10.2.3
    setupPluginVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.3.4"); // Will upgrade to 10.3.5

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
                                                     productsInfo({ products[0], products[1] }) };

    auto configurationData = configData(defaultSettings());
    Common::Policy::UpdateSettings previousConfigurationData;
    configurationData.verifySettingsAreValid();
    DownloadReport previousDownloadReport = DownloadReportBuilder::Report("Not assigned");

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

    EXPECT_CALL(fileSystemMock, isFile("update_config.json"))
        .Times(1)
        .WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, readFile("update_config.json")).Times(0);
    EXPECT_CALL(fileSystemMock,
                writeFile(HasSubstr("update_config.json"), "content"))
        .Times(0);
    setupBaseVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.2.2"); // Will upgrade to 10.2.3
    setupPluginVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.3.4"); // Will upgrade to 10.3.5

    int counter = 0;
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        [&counter, this, &fileSystemMock]() {
          if (counter == 0 || counter == 3) {
            counter++;
            return mockSystemctlStatus();
          } else if (counter == 1) {
            counter++;
            return mockBaseInstall(fileSystemMock);
          } else if (counter == 2) {
            counter++;
            return mockEDRInstall(fileSystemMock);
          } else {
            counter++;
            return mockSystemctlStart();
          }
        });

    ProductReportVector productReports = defaultProductReports();
    productReports[0].productStatus = ProductReport::ProductStatus::Upgraded;
    productReports[1].productStatus = ProductReport::ProductStatus::Upgraded;

    // if up to distribution passed, warehouse never returns error = true
    EXPECT_CALL(*mockSdds3Repo_, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockSdds3Repo_, tryConnect(_, _, _))
        .WillOnce(Return(true)); // successful tryConnect call
    EXPECT_CALL(fileSystemMock,
                recursivelyDeleteContentsOfDirectory(
                    "/opt/sophos-spl/base/update/cache/primarywarehouse"))
        .RetiresOnSaturation();
    EXPECT_CALL(fileSystemMock,
                recursivelyDeleteContentsOfDirectory(
                    "/opt/sophos-spl/base/update/cache/primary"))
        .RetiresOnSaturation();
    EXPECT_CALL(
        fileSystemMock,
        isDirectory("/opt/sophos-spl/base/update/cache/primarywarehouse"))
        .WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock,
                isDirectory("/opt/sophos-spl/base/update/cache/primary"))
        .WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, synchronize(_, _, _)).WillOnce(Return(true));
    EXPECT_CALL(*mockSdds3Repo_, distribute());
    EXPECT_CALL(*mockSdds3Repo_, purge());
    EXPECT_CALL(*mockSdds3Repo_, setWillInstall(_)).Times(2);
    // the real warehouse will set DistributePath after distribute to the
    // products
    products[0].setDistributePath(
        "/opt/sophos-spl/base/update/cache/sdds3primary/"
        "ServerProtectionLinux-Base-component");
    products[1].setDistributePath(
        "/opt/sophos-spl/base/update/cache/sdds3primary/"
        "ServerProtectionLinux-Plugin-EDR");
    EXPECT_CALL(*mockSdds3Repo_, getProducts())
        .Times(2)
        .WillRepeatedly(Return(products));
    EXPECT_CALL(*mockSdds3Repo_, getSourceURL());
    EXPECT_CALL(*mockSdds3Repo_, listInstalledProducts)
        .WillOnce(Return(productsInfo({products[0], products[1]})));
    EXPECT_CALL(*mockSdds3Repo_, listInstalledSubscriptions)
        .WillOnce(Return(subscriptionsInfo({products[0], products[1]})));
    TestSdds3RepositoryHelper::replaceSdds3RepositoryCreator(
        [this]() { return std::move(mockSdds3Repo_); });
    Sdds3SimplifiedDownloadReport expectedDownloadReport{
        RepositoryStatus::SUCCESS, "", productReports,
        productsInfo({products[0], products[1]})};

    auto configurationData = configData(defaultSettings());
    Common::Policy::UpdateSettings previousConfigurationData;
    configurationData.verifySettingsAreValid();
    DownloadReport previousDownloadReport =
        DownloadReportBuilder::Report("Not assigned");

    auto calculatedReport = SulDownloader::runSULDownloader(
        configurationData, previousConfigurationData, previousDownloadReport);
    EXPECT_PRED_FORMAT2(downloadReportSimilar, expectedDownloadReport,
                        calculatedReport);
    auto productsAndSubscriptions = calculatedReport.getProducts();
    ASSERT_EQ(productsAndSubscriptions.size(), 2);
    EXPECT_EQ(productsAndSubscriptions[0].rigidName, products[0].getLine());
    EXPECT_EQ(productsAndSubscriptions[1].rigidName, products[1].getLine());
}

TEST_F(SULDownloaderSdds3Test,
       runSULDownloader_SuccessfulFreshInstallShouldCopyConfigIntoPlace) {
    auto &fileSystemMock = setupFileSystemAndGetMock(1, 1, 0);
    DownloadedProductVector products = defaultProducts();

    for (auto &product : products) {
        product.setProductHasChanged(true);
    }

    products[0].setDistributePath(
        "/opt/sophos-spl/base/update/cache/sdds3primary/"
        "ServerProtectionLinux-Base-component");
    products[1].setDistributePath(
        "/opt/sophos-spl/base/update/cache/sdds3primary/"
        "ServerProtectionLinux-Plugin-EDR");
    std::string everest_installer =
        "/opt/sophos-spl/base/update/cache/sdds3primary/"
        "ServerProtectionLinux-Base-component/install.sh";
    std::string plugin_installer =
        "/opt/sophos-spl/base/update/cache/sdds3primary/"
        "ServerProtectionLinux-Plugin-EDR/install.sh";
    EXPECT_CALL(fileSystemMock, exists(everest_installer))
        .WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory(everest_installer))
        .WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(everest_installer)).Times(1);
    EXPECT_CALL(fileSystemMock, exists(plugin_installer))
        .WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory(plugin_installer))
        .WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(plugin_installer)).Times(1);
    std::vector<std::string> emptyFileList;
    std::string uninstallPath =
        "/opt/sophos-spl/base/update/var/installedproducts";
    EXPECT_CALL(fileSystemMock, isDirectory(uninstallPath))
        .WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, listFiles(uninstallPath))
        .WillOnce(Return(emptyFileList));
    EXPECT_CALL(
        fileSystemMock,
        exists("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini"))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(
        fileSystemMock,
        isFile("/opt/sophos-spl/base/update/var/sdds3_override_settings.ini"))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(
        fileSystemMock,
        readLines(
            "/opt/sophos-spl/base/update/var/sdds3_override_settings.ini"))
        .WillRepeatedly(Return(defaultOverrideSettings()));
    EXPECT_CALL(fileSystemMock, getSystemCommandExecutablePath(_))
        .WillRepeatedly(Return("systemctl"));
    EXPECT_CALL(fileSystemMock, isFile("update_config.json"))
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, readFile("update_config.json"))
        .Times(1)
        .WillOnce(Return("content"));
    EXPECT_CALL(fileSystemMock,
                writeFile(HasSubstr("update_config.json"), "content"))
        .Times(1);
    setupBaseVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.2.2"); // Will upgrade to 10.2.3
    setupPluginVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.3.4"); // Will upgrade to 10.3.5


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
                                                     productsInfo({ products[0], products[1] }) };

    auto configurationData = configData(defaultSettings());
    Common::Policy::UpdateSettings previousConfigurationData;
    configurationData.verifySettingsAreValid();
    DownloadReport previousDownloadReport = DownloadReportBuilder::Report("Not assigned");

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

    setupBaseVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.2.3");
    setupPluginVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.3.5");

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
                                                     productsInfo({ products[0], products[1] }) };

    auto configurationData = configData(defaultSettings());
    Common::Policy::UpdateSettings previousConfigurationData;
    configurationData.verifySettingsAreValid();
    DownloadReport previousDownloadReport = DownloadReportBuilder::Report("Not assigned");

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

    setupBaseVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.2.3");
    setupPluginVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.3.5");

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
                                                          productsInfo({ products[0], products[1] }) };

    auto configurationData = configData(defaultSettings());
    Common::Policy::UpdateSettings previousConfigurationData;
    configurationData.verifySettingsAreValid();
    DownloadReport previousDownloadReport = DownloadReportBuilder::Report("Not assigned");

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

    setupBaseVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.2.3");
    setupPluginVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.3.5");

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
                                                          productsInfo({ products[0], products[1] }) };

    auto configurationData = configData(defaultSettings());
    Common::Policy::UpdateSettings previousConfigurationData;
    configurationData.verifySettingsAreValid();
    DownloadReport previousDownloadReport = DownloadReportBuilder::Report("Not assigned");

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

    setupBaseVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.2.4"); // Will downgrade to 10.2.3
    setupPluginVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.3.6"); // Will downgrade to 10.3.5

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
                                                     productsInfo({ products[0], products[1] }) };

    auto configurationData = configData(defaultSettings());
    Common::Policy::UpdateSettings previousConfigurationData;
    configurationData.verifySettingsAreValid();
    DownloadReport previousDownloadReport = DownloadReportBuilder::Report("Not assigned");

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

    setupBaseVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.2.3");
    setupPluginVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.3.6"); // Will downgrade to 10.3.5

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
                                                     productsInfo({ products[0], products[1] }) };

    auto configurationData = configData(defaultSettings());
    Common::Policy::UpdateSettings previousConfigurationData;
    configurationData.verifySettingsAreValid();
    DownloadReport previousDownloadReport = DownloadReportBuilder::Report("Not assigned");

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

    setupBaseVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.2.3");
    setupPluginVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.3.5");

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
                                                     productsInfo({ products[0], products[1] }) };

    auto copyProduct = expectedDownloadReport.Products[0];
    expectedDownloadReport.Products.clear();
    expectedDownloadReport.Products.push_back(copyProduct);

    auto configurationData = configData(defaultSettings());
    Common::Policy::UpdateSettings previousConfigurationData;
    configurationData.verifySettingsAreValid();
    DownloadReport previousDownloadReport = DownloadReportBuilder::Report("Not assigned");

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

    setupBaseVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.2.3");
    setupPluginVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.3.5");

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
                                                     productsInfo({ products[0], products[1] }) };

    auto previousConfigurationData = configData(defaultSettings());
    auto configurationData = configData(newSubscriptionSettings());

    configurationData.verifySettingsAreValid();
    previousConfigurationData.verifySettingsAreValid();

    TimeTracker timeTracker;
    timeTracker.setStartTime(std::time_t(0));
    

    DownloadReport previousDownloadReport =
        DownloadReportBuilder::Report("", products, {}, {}, &timeTracker, DownloadReportBuilder::VerifyState::VerifyCorrect);

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

    setupBaseVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.2.3");
    setupPluginVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.3.5");

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
                                                     productsInfo({ products[0], products[1] }) };

    auto previousConfigurationData = configData(defaultSettings());
    auto configurationData = configData(newFeatureSettings());

    configurationData.verifySettingsAreValid();
    previousConfigurationData.verifySettingsAreValid();

    TimeTracker timeTracker;
    timeTracker.setStartTime(std::time_t(0));
    

    DownloadReport previousDownloadReport =
        DownloadReportBuilder::Report("", products, {}, {}, &timeTracker, DownloadReportBuilder::VerifyState::VerifyCorrect);

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

    setupBaseVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.2.3");
    setupPluginVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.3.5");

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
                                                     productsInfo({ products[0], products[1] }) };

    auto previousConfigurationData = configData(newSubscriptionSettings());
    auto configurationData = configData(newSubscriptionSettings("Plugin_2"));

    configurationData.verifySettingsAreValid();
    previousConfigurationData.verifySettingsAreValid();

    TimeTracker timeTracker;
    timeTracker.setStartTime(std::time_t(0));
    

    DownloadReport previousDownloadReport =
        DownloadReportBuilder::Report("", products, {}, {}, &timeTracker, DownloadReportBuilder::VerifyState::VerifyCorrect);

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

    setupBaseVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.2.3");
    setupPluginVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.3.5");

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
                                                     productsInfo({ products[0], products[1] }) };

    auto previousConfigurationData = configData(newFeatureSettings());
    auto configurationData = configData(newFeatureSettings("DIFF_FEATURE"));

    configurationData.verifySettingsAreValid();
    previousConfigurationData.verifySettingsAreValid();

    TimeTracker timeTracker;
    timeTracker.setStartTime(std::time_t(0));
    

    DownloadReport previousDownloadReport =
        DownloadReportBuilder::Report("", products, {}, {}, &timeTracker, DownloadReportBuilder::VerifyState::VerifyCorrect);

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

    setupBaseVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.2.3");
    setupPluginVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.3.5");

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
                                                     productsInfo({ products[0], products[1] }) };

    auto configurationData = configData(defaultSettings());
    Common::Policy::UpdateSettings previousConfigurationData;
    configurationData.verifySettingsAreValid();
    previousConfigurationData.verifySettingsAreValid();
    DownloadReport previousDownloadReport = DownloadReportBuilder::Report("Not assigned");

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
    EXPECT_CALL(fileSystemMock, getSystemCommandExecutablePath(_)).WillRepeatedly(Return("systemctl"));
    TestSdds3RepositoryHelper::replaceSdds3RepositoryCreator([this]() { return std::move(mockSdds3Repo_); });

    setupBaseVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.2.3");
    setupPluginVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.3.5");

    Sdds3SimplifiedDownloadReport expectedDownloadReport{ RepositoryStatus::SUCCESS,
                                                     "",
                                                     productReports,
                                                     productsInfo({ products[0], products[1] }) };

    auto configurationData = configData(defaultSettings());
    auto previousConfigurationData = configData(defaultSettings());
    configurationData.verifySettingsAreValid();

    TimeTracker timeTracker;
    timeTracker.setStartTime(std::time_t(0));
    
    DownloadReport previousDownloadReport =
        DownloadReportBuilder::Report("", products, {}, {}, &timeTracker, DownloadReportBuilder::VerifyState::VerifyCorrect);

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
    setupBaseVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.2.3");
    setupPluginVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.3.5");

    Sdds3SimplifiedDownloadReport expectedDownloadReport{ RepositoryStatus::SUCCESS,
                                                     "",
                                                     productReports,
                                                     productsInfo({ products[0], products[1] }) };

    auto configurationData = configData(defaultSettings());
    auto previousConfigurationData = configData(defaultSettings());
    configurationData.verifySettingsAreValid();

    TimeTracker timeTracker;
    timeTracker.setStartTime(std::time_t(0));
    
    DownloadReport previousDownloadReport =
        DownloadReportBuilder::Report("", products, {}, {}, &timeTracker, DownloadReportBuilder::VerifyState::VerifyCorrect);

    auto actualDownloadReport = SulDownloader::runSULDownloader(configurationData, previousConfigurationData, previousDownloadReport, true);
    EXPECT_TRUE(actualDownloadReport.isSupplementOnlyUpdate());

    EXPECT_PRED_FORMAT2(
        downloadReportSimilar,
        expectedDownloadReport,
        actualDownloadReport);
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
    setupBaseVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.2.3");
    setupPluginVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.3.5");

    TimeTracker timeTracker;
    timeTracker.setStartTime(std::time_t(0));
    
    DownloadReport downloadReport =
        DownloadReportBuilder::Report("", products, {}, {}, &timeTracker, DownloadReportBuilder::VerifyState::VerifyCorrect);
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
    DownloadReport previousDownloadReport = DownloadReportBuilder::Report("Not assigned");

    auto report =
        SulDownloader::runSULDownloader(configurationData, previousConfigurationData, previousDownloadReport);

    //We wont writeInstalledFeatures if return code is non zero
    EXPECT_EQ(report.getStatus(), RepositoryStatus::DOWNLOADFAILED);

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
    DownloadReport previousDownloadReport = DownloadReportBuilder::Report("Not assigned");

    auto report =
        SulDownloader::runSULDownloader(configurationData, previousConfigurationData, previousDownloadReport);

    //We wont writeInstalledFeatures if return code is non zero
    EXPECT_EQ(report.getStatus(), RepositoryStatus::DOWNLOADFAILED);

    std::string errStd = testing::internal::GetCapturedStderr();
    ASSERT_THAT(errStd, ::testing::HasSubstr("The requested fixed version is not available on SDDS3"));
}

TEST_F(SULDownloaderSdds3Test, runSULDownloader_NonSupplementOnlyClearsAwaitScheduledUpdateFlagAndTriesToInstall)
{
    auto& fileSystemMock = setupFileSystemAndGetMock(0, 1, 0);

    // Expect an upgrade
    setupBaseVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.2.2"); // Will upgrade to 10.2.3
    setupPluginVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 10.3.4"); // Will upgade to 10.3.5

    auto settings = defaultSettings();
    auto configurationData = configData(settings);
    configurationData.verifySettingsAreValid();
    Common::Policy::UpdateSettings previousConfigurationData;
    const auto products = defaultProducts();
    TimeTracker timeTracker;
    DownloadReport previousDownloadReport =
        DownloadReportBuilder::Report("", products, {}, {},
                               &timeTracker, DownloadReportBuilder::VerifyState::VerifyCorrect);

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
    setupBaseVersionFileCalls(mockFileSystem, "PRODUCT_VERSION = 1.2.2.999"); // Will upgrade to 10.2.3
    setupPluginVersionFileCalls(mockFileSystem, "PRODUCT_VERSION = 1.2.2.999"); // Will upgrade to 10.3.5

    auto settings = defaultSettings();
    auto configurationData = configData(settings);
    configurationData.verifySettingsAreValid();
    Common::Policy::UpdateSettings previousConfigurationData;
    const auto products = defaultProducts();
    TimeTracker timeTracker;
    DownloadReport previousDownloadReport =
        DownloadReportBuilder::Report("", products, {}, {}, &timeTracker, DownloadReportBuilder::VerifyState::VerifyCorrect);

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
    setupBaseVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 1.2.3.0"); // Will upgrade to 10.2.3
    setupPluginVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 1.2.3.0"); // Will upgrade to 10.3.5

    auto settings = defaultSettings();
    auto configurationData = configData(settings);
    configurationData.verifySettingsAreValid();
    Common::Policy::UpdateSettings previousConfigurationData;
    const auto products = defaultProducts();
    TimeTracker timeTracker;
    DownloadReport previousDownloadReport =
        DownloadReportBuilder::Report("", products, {}, {}, &timeTracker, DownloadReportBuilder::VerifyState::VerifyCorrect);

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
    setupPluginVersionFileCalls(fileSystemMock, "PRODUCT_VERSION = 1.2.2.999"); // Will upgrade to 10.3.5

    auto settings = defaultSettings();
    auto configurationData = configData(settings);
    configurationData.verifySettingsAreValid();
    Common::Policy::UpdateSettings previousConfigurationData;
    const auto products = defaultProducts();
    TimeTracker timeTracker;
    DownloadReport previousDownloadReport =
        DownloadReportBuilder::Report("", products, {}, {}, &timeTracker, DownloadReportBuilder::VerifyState::VerifyCorrect);

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
    setupPluginVersionFileCalls(mockFileSystem, "PRODUCT_VERSION = 1.2.2.999"); // Will upgrade to 10.3.5

    auto settings = defaultSettings();
    auto configurationData = configData(settings);
    configurationData.verifySettingsAreValid();
    Common::Policy::UpdateSettings previousConfigurationData;
    const auto products = defaultProducts();
    TimeTracker timeTracker;
    DownloadReport previousDownloadReport =
        DownloadReportBuilder::Report("", products, {}, {}, &timeTracker, DownloadReportBuilder::VerifyState::VerifyCorrect);

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
#ifdef SPL_BAZEL
    EXPECT_TRUE(appenderContains("Failed to process json message with error: INVALID_ARGUMENT: invalid JSON"));
#else
    EXPECT_TRUE(appenderContains("Failed to process json message with error: INVALID_ARGUMENT:Expected a value.\n<?xml version=\"1.0\"?\n^"));
#endif
}