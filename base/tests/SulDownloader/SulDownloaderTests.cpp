/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

/**
* Component tests to SULDownloader mocking out WarehouseRepository
*/
#include <modules/Common/FileSystem/IFileSystemException.h>
#include <modules/Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <modules/Common/ApplicationConfiguration/IApplicationPathManager.h>
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "modules/SulDownloader/ConfigurationData.h"
#include "modules/Common/UtilityImpl/MessageUtility.h"
#include "tests/Common/ProcessImpl/MockProcess.h"
#include "tests/Common/FileSystemImpl/MockFileSystem.h"
#include "MockWarehouseRepository.h"
#include "ConfigurationSettings.pb.h"
#include "SulDownloader/SulDownloader.h"
#include "Common/ProcessImpl/ArgcAndEnv.h"
#include "TestWarehouseHelper.h"
#include "SulDownloader/SulDownloaderException.h"
#include "SulDownloader/DownloadReport.h"
#include "Common/ProcessImpl/ProcessImpl.h"
#include "Common/FileSystemImpl/FileSystemImpl.h"
#include "SulDownloader/VersigImpl.h"
#include "MockVersig.h"

using SulDownloaderProto::ConfigurationSettings;

struct SimplifiedDownloadReport
{
    SulDownloader::WarehouseStatus Status;
    std::string Description;
    std::vector<SulDownloader::ProductReport> Products;
};

class SULDownloaderTest : public ::testing::Test
{
public:
    /**
     * Setup directories and files expected by SULDownloader to enable its execution.
     * Use TempDir
     */
    void SetUp() override
    {
        Test::SetUp();

        SulDownloader::VersigFactory::instance().replaceCreator([](){
            auto versig = new NiceMock<MockVersig>();
            ON_CALL(*versig, verify(_,_)).WillByDefault(Return(SulDownloader::IVersig::VerifySignature::SIGNATURE_VERIFIED));
            return std::unique_ptr<SulDownloader::IVersig>(versig);
        });
        m_mockptr = nullptr;
    }

    /**
     * Remove the temporary directory.
     */
    void TearDown() override
    {
        Common::FileSystem::restoreFileSystem();
        SulDownloader::VersigFactory::instance().restoreCreator();
        TestWarehouseHelper helper;
        helper.restoreWarehouseFactory();
        Test::TearDown();
    }

    ConfigurationSettings defaultSettings()
    {
        ConfigurationSettings settings;
        Credentials credentials;

        settings.add_sophosurls("http://ostia.eng.sophosinvalid/latest/Virt-vShieldInvalid");
        //settings.add_updatecache("http://192.168.10.10:800/latest/Virt-vShieldInvalid");
        settings.mutable_credential()->set_username("administrator");
        settings.mutable_credential()->set_password("password");
        settings.mutable_proxy()->set_url("noproxy:");
        settings.mutable_proxy()->mutable_credential()->set_username("");
        settings.mutable_proxy()->mutable_credential()->set_password("");
        settings.set_baseversion("10");
        settings.set_releasetag("RECOMMENDED");
        settings.set_primary("Everest-Base");
        //settings.add_fullnames("Everest-Plugin-Update-Cache");
        settings.add_prefixnames("Everest-Plugins");

        settings.set_certificatepath("/installroot/update/certificates");
        settings.set_installationrootpath("/installroot");
        settings.set_systemsslpath("/installroot/etc/ssl");
        settings.set_cacheupdatesslpath("/installroot/etc/cache/ssl");

        return settings;
    }

    std::string jsonSettings(const ConfigurationSettings & configSettings)
    {
        return Common::UtilityImpl::MessageUtility::protoBuf2Json( configSettings );
    }

    SulDownloader::ConfigurationData configData( const ConfigurationSettings & configSettings)
    {
        std::string json_output = jsonSettings( configSettings );
        return SulDownloader::ConfigurationData::fromJsonSettings(json_output);
    }

    std::vector<SulDownloader::DownloadedProduct> defaultProducts()
    {
        std::vector<SulDownloader::DownloadedProduct> products;
        for( auto & metadata: defaultMetadata())
        {
            SulDownloader::DownloadedProduct product(metadata);
            product.setPostUpdateInstalledVersion(metadata.getVersion());
            product.setDistributePath("/installroot/update/cache/primary");
            products.push_back(product);
        }
        return products;
    }

    std::vector<SulDownloader::ProductMetadata> defaultMetadata()
    {
        SulDownloader::ProductMetadata base;
        base.setDefaultHomePath("everest");
        base.setLine("Everest-Base");
        base.setName("Everest-Base-Product");
        base.setVersion("10.2.3");
        base.setTags({{"RECOMMENDED","10", "Base-label"}});

        SulDownloader::ProductMetadata plugin;
        plugin.setDefaultHomePath("everest-plugin-a");
        plugin.setLine("Everest-Plugins-A");
        plugin.setName("Everest-Plugins-A-Product");
        plugin.setVersion("10.3.5");
        plugin.setTags({{"RECOMMENDED","10", "Plugin-label"}});

        return std::vector<SulDownloader::ProductMetadata>{base,plugin};


    }

    std::vector<SulDownloader::ProductReport> defaultProductReports()
    {
        SulDownloader::ProductReport base;
        base.name = "Everest-Base-Product";
        base.rigidName = "Everest-Base";
        base.downloadedVersion = "10.2.3";
        base.installedVersion = base.downloadedVersion;
        SulDownloader::ProductReport plugin;
        plugin.name = "Everest-Plugins-A-Product";
        plugin.rigidName = "Everest-Plugins-A";
        plugin.downloadedVersion = "10.3.5";
        plugin.installedVersion = plugin.downloadedVersion;
        return std::vector<SulDownloader::ProductReport>{base,plugin};

    }


    MockWarehouseRepository & warehouseMocked()
    {
        if (m_mockptr == nullptr)
        {
            m_mockptr = new MockWarehouseRepository();
            TestWarehouseHelper helper;
            helper.replaceWarehouseCreator([this](const ConfigurationData & ){return std::unique_ptr<SulDownloader::IWarehouseRepository>(this->m_mockptr);});
        }
        return *m_mockptr;
    }


    MockFileSystem & setupFileSystemAndGetMock()
    {
        auto filesystemMock = new MockFileSystem();
        EXPECT_CALL(*filesystemMock, isDirectory("/installroot")).WillOnce(Return(true));
        EXPECT_CALL(*filesystemMock, isDirectory("/installroot/base/update/cache/primarywarehouse")).WillOnce(
                Return(true));
        EXPECT_CALL(*filesystemMock, isDirectory("/installroot/base/update/cache/primary")).WillOnce(Return(true));
        EXPECT_CALL(*filesystemMock, exists(_)).WillRepeatedly(Return(true));
        EXPECT_CALL(*filesystemMock, join(_,_)).WillRepeatedly(Invoke([](const std::string& a, const std::string&b){return a + "/" + b; }));
        auto pointer = filesystemMock;
        Common::FileSystem::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));
        return *pointer;
    }




    ::testing::AssertionResult downloadReportSimilar( const char* m_expr,
                                                      const char* n_expr,
                                                      const SimplifiedDownloadReport & expected,
                                                      const SulDownloader::DownloadReport & resulted)
    {
        std::stringstream s;
        s<< m_expr << " and " << n_expr << " failed: ";

        if ( expected.Status != resulted.getStatus())
        {
            return ::testing::AssertionFailure() << s.str() << " status differ: \n expected: "
                                                 <<  SulDownloader::toString(expected.Status)
                                                 << "\n result: " <<  SulDownloader::toString(resulted.getStatus());
        }
        if( expected.Description != resulted.getDescription())
        {
            return ::testing::AssertionFailure() << s.str() << " Description differ: \n expected: "
                                                 <<  expected.Description
                                                 << "\n result: " <<  resulted.getDescription();
        }

        std::string expectedProductsSerialized = productsToString(expected.Products);
        std::string resultedProductsSerialized = productsToString(resulted.getProducts());
        if ( expectedProductsSerialized != resultedProductsSerialized)
        {
            return ::testing::AssertionFailure() << s.str() << " Different products. \n expected: "
                                                 <<  expectedProductsSerialized
                                                 << "\n result: " << resultedProductsSerialized;
        }

        return ::testing::AssertionSuccess();
    }

    std::string productsToString(const std::vector<SulDownloader::ProductReport> & productsReport)
    {
        std::stringstream stream;
        for( auto & productReport : productsReport)
        {
            stream << "name: " << productReport.name
                   << ", version: " << productReport.downloadedVersion
                   << ", error: " << productReport.errorDescription
                   << '\n';
        }
        std::string serialized = stream.str();
        if ( serialized.empty())
        {
            return "{empty}";
        }
        return serialized;
    }


    std::string everestpath() const
    {
        return Common::FileSystem::join(
                Common::ApplicationConfiguration::applicationPathManager().getLocalDistributionRepository(), "everest"
        );
    }

    std::string everestpluginpath() const
    {
        return Common::FileSystem::join(
                Common::ApplicationConfiguration::applicationPathManager().getLocalDistributionRepository(),
                "everest-plugin-a"
        );
    }

protected:
    MockWarehouseRepository * m_mockptr = nullptr;
};

TEST_F( SULDownloaderTest, configurationDataVerificationOfDefaultSettingsReturnsTrue )
{
    setupFileSystemAndGetMock();
    SulDownloader::ConfigurationData confData = configData( defaultSettings());
    confData.verifySettingsAreValid();
    EXPECT_TRUE( confData.isVerified());
}

TEST_F( SULDownloaderTest, main_entry_InvalidArgumentsReturnsTheCorrectErrorCode)
{
    auto filesystemMock = new MockFileSystem();
    Common::FileSystem::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    int expectedErrorCode = -2;

    char ** argsNotUsed = nullptr;
    EXPECT_EQ( SulDownloader::main_entry(2, argsNotUsed), -1);
    EXPECT_EQ( SulDownloader::main_entry(1, argsNotUsed), -1);
    char * inputFileDoesNotExist [] = {const_cast<char*>("SulDownloader"),
                                       const_cast<char*>("inputfiledoesnotexists.json"),
                                       const_cast<char*>("createoutputpath.json")};
    EXPECT_CALL(*filesystemMock, readFile("inputfiledoesnotexists.json")).WillOnce(Throw(Common::FileSystem::IFileSystemException("")));

    EXPECT_EQ( SulDownloader::main_entry(3, inputFileDoesNotExist), -2);

    EXPECT_CALL(*filesystemMock, readFile("input.json")).WillOnce(Return(jsonSettings(defaultSettings())) );

    EXPECT_CALL(*filesystemMock, isDirectory("/installroot/directorypath")).WillOnce(Return(true));
    // directory can not be replaced by file
    Common::ProcessImpl::ArgcAndEnv args("SulDownloader", {"input.json", "/installroot/directorypath"}, {});

    EXPECT_EQ( SulDownloader::main_entry(3, args.argc()), expectedErrorCode);
}

/**
 * To be fixed in LINUXEP-6174, it should not call currentdirectory once that ticket has been fixed.
 * Breaks when build dir and /tmp are on different partitions.
 */
TEST_F( SULDownloaderTest, main_entry_onSuccessCreatesReportContainingExpectedSuccessResult)
{
    auto & fileSystemMock = setupFileSystemAndGetMock();
    MockWarehouseRepository & mock = warehouseMocked();

    auto products = defaultProducts();
    products[0].setProductHasChanged(false);
    products[1].setProductHasChanged(false);

    EXPECT_CALL(mock, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(mock, synchronize(_));
    EXPECT_CALL(mock, distribute());
    EXPECT_CALL(mock, getProducts()).WillOnce(Return(products));


    // it should not depend on currentWorkingDirectory:  	LINUXEP-6153
    EXPECT_CALL(fileSystemMock, currentWorkingDirectory()).Times(0);
    EXPECT_CALL(fileSystemMock, dirName("/dir/output.json")).WillOnce(Return("/dir"));
    EXPECT_CALL(fileSystemMock, isDirectory("/dir/output.json")).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, isDirectory("/dir")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, readFile("/dir/input.json")).WillOnce(Return(jsonSettings(defaultSettings())));
    EXPECT_CALL(fileSystemMock, writeFileAtomically("/dir/output.json", ::testing::HasSubstr(
            SulDownloader::toString(SulDownloader::WarehouseStatus::SUCCESS)), "/installroot/tmp"
    ));



    Common::ProcessImpl::ArgcAndEnv args("SulDownloader", {"/dir/input.json", "/dir/output.json"}, {});

    EXPECT_EQ( SulDownloader::main_entry(3, args.argc()), 0);


}

// the other execution paths were covered in main_entry_* tests.
TEST_F( SULDownloaderTest, fileEntriesAndRunDownloaderThrowIfCannotCreateOutputFile)
{
    auto filesystemMock = new MockFileSystem();
    Common::FileSystem::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));
    EXPECT_CALL(*filesystemMock, readFile("/dir/input.json")).WillOnce(Return(jsonSettings(defaultSettings())));
    EXPECT_CALL(*filesystemMock, isDirectory("/dir/path/that/cannot/be/created/output.json")).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, dirName("/dir/path/that/cannot/be/created/output.json")).WillOnce(Return("/dir/path/that/cannot/be/created/"));
    EXPECT_CALL(*filesystemMock, isDirectory("/dir/path/that/cannot/be/created/")).WillOnce(Return(false));

    EXPECT_THROW(SulDownloader::fileEntriesAndRunDownloader(
                 "/dir/input.json", "/dir/path/that/cannot/be/created/output.json"),
            SulDownloader::SulDownloaderException);
}

// configAndRunDownloader
TEST_F( SULDownloaderTest, configAndRunDownloaderInvalidSettingsReportError_WarehouseStatus_UNSPECIFIED)
{
    auto filesystemMock = new MockFileSystem();
    Common::FileSystem::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));
    std::string reportContent;
    int exitCode =0;
    auto settings = defaultSettings();
    settings.clear_sophosurls(); // no sophos urls, the system can not connect to warehouses
    std::string settingsString = jsonSettings(settings);

    std::tie(exitCode,reportContent) = SulDownloader::configAndRunDownloader(settingsString);

    EXPECT_NE(exitCode, 0 );
    EXPECT_THAT( reportContent, ::testing::Not(::testing::HasSubstr( SulDownloader::toString(SulDownloader::WarehouseStatus::SUCCESS))));
    EXPECT_THAT( reportContent, ::testing::HasSubstr( SulDownloader::toString(SulDownloader::WarehouseStatus::UNSPECIFIED)));
}

// runSULDownloader
TEST_F( SULDownloaderTest, runSULDownloader_WarehouseConnectionFailureShouldCreateValidConnectionFailureReport)
{
    setupFileSystemAndGetMock();
    MockWarehouseRepository & mock = warehouseMocked();
    SulDownloader::WarehouseError wError;
    wError.Description = "Error description";
    wError.status = SulDownloader::WarehouseStatus::CONNECTIONERROR;
    std::string statusError = SulDownloader::toString(wError.status);
    std::vector<SulDownloader::DownloadedProduct> emptyProducts;

    EXPECT_CALL(mock, hasError()).WillOnce(Return(true)).WillRepeatedly(Return(true));
    EXPECT_CALL(mock, getError()).WillOnce(Return(wError));
    EXPECT_CALL(mock, getProducts()).WillOnce(Return(emptyProducts));

    SimplifiedDownloadReport expectedDownloadReport{wError.status, wError.Description,{}};

    ConfigurationData configurationData = configData(defaultSettings());
    configurationData.verifySettingsAreValid();

    EXPECT_PRED_FORMAT2( downloadReportSimilar, expectedDownloadReport, SulDownloader::runSULDownloader(configurationData));
}


TEST_F( SULDownloaderTest, runSULDownloader_WarehouseSynchronizationFailureShouldCreateValidSyncronizationFailureReport)
{
    setupFileSystemAndGetMock();
    MockWarehouseRepository & mock = warehouseMocked();
    SulDownloader::WarehouseError wError;
    wError.Description = "Error description";
    wError.status = SulDownloader::WarehouseStatus::PACKAGESOURCEMISSING;
    std::string statusError = SulDownloader::toString(wError.status);
    std::vector<SulDownloader::DownloadedProduct> emptyProducts;

    EXPECT_CALL(mock, hasError()).WillOnce(Return(false)) // connection
                                 .WillOnce(Return(true)) // synchronization
                                 .WillRepeatedly(Return(true));
    EXPECT_CALL(mock, synchronize(_));
    EXPECT_CALL(mock, getError()).WillOnce(Return(wError));
    EXPECT_CALL(mock, getProducts()).WillOnce(Return(emptyProducts));

    SimplifiedDownloadReport expectedDownloadReport{wError.status, wError.Description,{}};

    ConfigurationData configurationData = configData(defaultSettings());
    configurationData.verifySettingsAreValid();

    EXPECT_PRED_FORMAT2( downloadReportSimilar, expectedDownloadReport, SulDownloader::runSULDownloader(configurationData));

}

/**
 * Simulate the error in distributing only one of the products
 */
TEST_F( SULDownloaderTest, runSULDownloader_onDistributeFailure)
{
    setupFileSystemAndGetMock();
    MockWarehouseRepository & mock = warehouseMocked();
    SulDownloader::WarehouseError wError;
    wError.Description = "Error description";
    wError.status = SulDownloader::WarehouseStatus::DOWNLOADFAILED;
    std::string statusError = SulDownloader::toString(wError.status);
    std::vector<SulDownloader::DownloadedProduct> products = defaultProducts();

    SulDownloader::WarehouseError productError;
    productError.Description = "Product Error description";
    productError.status = SulDownloader::WarehouseStatus::DOWNLOADFAILED;

    products[0].setError(productError);

    std::vector<SulDownloader::ProductReport> productReports = defaultProductReports();
    productReports[0].errorDescription = productError.Description;
    productReports[0].installedVersion = "";


    EXPECT_CALL(mock, hasError()).WillOnce(Return(false)) // connection
                                 .WillOnce(Return(false)) // synchronization
                                 .WillOnce(Return(true)) // distribute
                                 .WillRepeatedly(Return(true));
    EXPECT_CALL(mock, synchronize(_));
    EXPECT_CALL(mock, distribute());
    EXPECT_CALL(mock, getError()).WillOnce(Return(wError));
    EXPECT_CALL(mock, getProducts()).WillOnce(Return(products));

    SimplifiedDownloadReport expectedDownloadReport{wError.status, wError.Description,productReports};

    ConfigurationData configurationData = configData(defaultSettings());
    configurationData.verifySettingsAreValid();

    EXPECT_PRED_FORMAT2( downloadReportSimilar, expectedDownloadReport, SulDownloader::runSULDownloader(configurationData));
}

TEST_F( SULDownloaderTest, runSULDownloader_WarehouseSynchronizationResultingInNoUpdateNeededShouldCreateValidSuccessReport)
{
    setupFileSystemAndGetMock();
    MockWarehouseRepository & mock = warehouseMocked();
    std::vector<SulDownloader::DownloadedProduct> products = defaultProducts();
    std::vector<SulDownloader::ProductReport> productReports = defaultProductReports();

    for ( auto & product : products)
    {
        product.setProductHasChanged(false);
    }

    EXPECT_CALL(mock, hasError()).WillRepeatedly(Return(false));

    EXPECT_CALL(mock, synchronize(_));
    EXPECT_CALL(mock, distribute());
    EXPECT_CALL(mock, getProducts()).WillOnce(Return(products));

    SimplifiedDownloadReport expectedDownloadReport{SulDownloader::WarehouseStatus::SUCCESS, "", productReports};

    ConfigurationData configurationData = configData(defaultSettings());
    configurationData.verifySettingsAreValid();

    EXPECT_PRED_FORMAT2( downloadReportSimilar, expectedDownloadReport, SulDownloader::runSULDownloader(configurationData));
}
/**
 * Simulate invalid signature
 *
 */
TEST_F( SULDownloaderTest, runSULDownloader_UpdateFailForInvalidSignature)
{
    setupFileSystemAndGetMock();
    MockWarehouseRepository & mock = warehouseMocked();

    std::vector<SulDownloader::DownloadedProduct> products = defaultProducts();

    for ( auto & product : products)
    {
        product.setProductHasChanged(true);
    }

    std::string everest_installer = "/installroot/update/cache/primary/everest/install.sh";
    std::string plugin_installer = "/installroot/update/cache/primary/everest-plugin-a/install.sh";
    int counter = 0;

    SulDownloader::VersigFactory::instance().replaceCreator([&counter](){
        auto versig = new StrictMock<MockVersig>();
        if ( counter++ == 0 )
        {
            EXPECT_CALL(*versig, verify("/installroot/update/certificates/rootca.crt",
                                        "/installroot/update/cache/primary/everest"
            ))
                    .WillOnce(Return(SulDownloader::IVersig::VerifySignature::SIGNATURE_VERIFIED));
        }
        else
        {
            EXPECT_CALL(*versig, verify("/installroot/update/certificates/rootca.crt",
                                        "/installroot/update/cache/primary/everest-plugin-a"
            ))
                    .WillOnce(Return(SulDownloader::IVersig::VerifySignature::SIGNATURE_FAILED));
        }


        return std::unique_ptr<SulDownloader::IVersig>(versig);
    });



    std::vector<SulDownloader::ProductReport> productReports = defaultProductReports();

    // if up to distribution passed, warehouse never returns error = true
    EXPECT_CALL(mock, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(mock, synchronize(_));
    EXPECT_CALL(mock, distribute());
    EXPECT_CALL(mock, getProducts()).WillOnce(Return(products));

    SimplifiedDownloadReport expectedDownloadReport{SulDownloader::WarehouseStatus::INSTALLFAILED,
                                                    "Update failed",productReports};

    expectedDownloadReport.Products[1].errorDescription = "Product Everest-Plugins-A failed signature verification";
    ConfigurationData configurationData = configData(defaultSettings());
    configurationData.verifySettingsAreValid();
    auto result = SulDownloader::runSULDownloader(configurationData);
    EXPECT_PRED_FORMAT2( downloadReportSimilar, expectedDownloadReport, result);
}



/**
 * Simulate error in installing base successfully but fail to install plugin
 */
TEST_F( SULDownloaderTest, runSULDownloader_PluginInstallationFailureShouldResultInValidInstalledFailedReport)
{
    auto & fileSystemMock = setupFileSystemAndGetMock();

    MockWarehouseRepository & mock = warehouseMocked();

    std::vector<SulDownloader::DownloadedProduct> products = defaultProducts();

    for ( auto & product : products)
    {
        product.setProductHasChanged(true);
    }

    std::string everest_installer = "/installroot/update/cache/primary/everest/install.sh";
    std::string plugin_installer = "/installroot/update/cache/primary/everest-plugin-a/install.sh";
    EXPECT_CALL(fileSystemMock, exists(everest_installer)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory(everest_installer)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(everest_installer)).Times(1);
    EXPECT_CALL(fileSystemMock, exists(plugin_installer)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory(plugin_installer)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(plugin_installer)).Times(1);

    int counter = 0;

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&counter](){
        if ( counter++ == 0 )
        {
            auto mockProcess = new MockProcess();
            EXPECT_CALL(*mockProcess, exec(HasSubstr("everest/install.sh"), _)).Times(1);
            EXPECT_CALL(*mockProcess, output()).WillOnce(Return("installing base"));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);

        } else
        {
            auto mockProcess = new MockProcess();
            EXPECT_CALL(*mockProcess, exec(HasSubstr("everest-plugin-a/install.sh"), _)).Times(1);
            EXPECT_CALL(*mockProcess, output()).WillOnce(Return("installing plugin\nsimulate failure"));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(5));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        }
    }
    );

    std::vector<SulDownloader::ProductReport> productReports = defaultProductReports();
    productReports[1].errorDescription = "Product Everest-Plugins-A failed to install";
    productReports[1].installedVersion = "";

    // if up to distribution passed, warehouse never returns error = true
    EXPECT_CALL(mock, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(mock, synchronize(_));
    EXPECT_CALL(mock, distribute());
    EXPECT_CALL(mock, getProducts()).WillOnce(Return(products));

    SimplifiedDownloadReport expectedDownloadReport{SulDownloader::WarehouseStatus::INSTALLFAILED,
                                                    "Update failed",productReports};

    ConfigurationData configurationData = configData(defaultSettings());
    configurationData.verifySettingsAreValid();

    EXPECT_PRED_FORMAT2( downloadReportSimilar, expectedDownloadReport, SulDownloader::runSULDownloader(configurationData));
}


/**
 * Simulate successful full update
 *
 */
TEST_F( SULDownloaderTest, runSULDownloader_SuccessfulFullUpdateShouldResultInValidSuccessReport)
{
    auto & fileSystemMock = setupFileSystemAndGetMock();
    MockWarehouseRepository & mock = warehouseMocked();

    std::vector<SulDownloader::DownloadedProduct> products = defaultProducts();

    for ( auto & product : products)
    {
        product.setProductHasChanged(true);
    }

    std::string everest_installer = "/installroot/update/cache/primary/everest/install.sh";
    std::string plugin_installer = "/installroot/update/cache/primary/everest-plugin-a/install.sh";
    EXPECT_CALL(fileSystemMock, exists(everest_installer)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory(everest_installer)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(everest_installer)).Times(1);
    EXPECT_CALL(fileSystemMock, exists(plugin_installer)).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isDirectory(plugin_installer)).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, makeExecutable(plugin_installer)).Times(1);

    int counter = 0;

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator([&counter](){
           if ( counter++ == 0 )
           {
               auto mockProcess = new MockProcess();
               EXPECT_CALL(*mockProcess, exec(HasSubstr("everest/install.sh"), _)).Times(1);
               EXPECT_CALL(*mockProcess, output()).WillOnce(Return("installing base"));
               EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
               return std::unique_ptr<Common::Process::IProcess>(mockProcess);

           } else
           {
               auto mockProcess = new MockProcess();
               EXPECT_CALL(*mockProcess, exec(HasSubstr("everest-plugin-a/install.sh"), _)).Times(1);
               EXPECT_CALL(*mockProcess, output()).WillOnce(Return("installing plugin"));
               EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
               return std::unique_ptr<Common::Process::IProcess>(mockProcess);
           }
       }
    );




    std::vector<SulDownloader::ProductReport> productReports = defaultProductReports();

    // if up to distribution passed, warehouse never returns error = true
    EXPECT_CALL(mock, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(mock, synchronize(_));
    EXPECT_CALL(mock, distribute());
    EXPECT_CALL(mock, getProducts()).WillOnce(Return(products));

    SimplifiedDownloadReport expectedDownloadReport{SulDownloader::WarehouseStatus::SUCCESS,
                                                    "",productReports};

    ConfigurationData configurationData = configData(defaultSettings());
    configurationData.verifySettingsAreValid();

    EXPECT_PRED_FORMAT2( downloadReportSimilar, expectedDownloadReport, SulDownloader::runSULDownloader(configurationData));
}

TEST_F( SULDownloaderTest, runSULDownloader_checkLogVerbosityVERBOSE)
{
    setupFileSystemAndGetMock();
    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();
    auto settings = defaultSettings();
    settings.clear_sophosurls();
    settings.clear_cacheupdatesslpath();
    settings.add_sophosurls("http://localhost/latest/donotexits");
    settings.set_loglevel( ConfigurationSettings::VERBOSE);
    ConfigurationData configurationData = configData(settings);
    configurationData.verifySettingsAreValid();
    auto downloadReport = SulDownloader::runSULDownloader(configurationData);
    std::string output = testing::internal::GetCapturedStdout();
    std::string errStd = testing::internal::GetCapturedStderr();
    ASSERT_THAT( output, ::testing::HasSubstr("Proxy used was"));
    ASSERT_THAT( errStd, ::testing::HasSubstr("Failed to connect to the warehouse"));
}

TEST_F( SULDownloaderTest, runSULDownloader_checkLogVerbosityNORMAL)
{
    setupFileSystemAndGetMock();
    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();
    auto settings = defaultSettings();
    settings.clear_sophosurls();
    settings.clear_cacheupdatesslpath();
    settings.add_sophosurls("http://localhost/latest/donotexits");
    settings.set_loglevel( ConfigurationSettings::NORMAL);
    ConfigurationData configurationData = configData(settings);
    configurationData.verifySettingsAreValid();
    auto downloadReport = SulDownloader::runSULDownloader(configurationData);
    std::string output = testing::internal::GetCapturedStdout();
    std::string errStd = testing::internal::GetCapturedStderr();
    ASSERT_THAT( output, ::testing::Not( ::testing::HasSubstr("Proxy used was")));
    ASSERT_THAT( errStd, ::testing::HasSubstr("Failed to connect to the warehouse"));
}


