/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

/**
* Component tests to SULDownloader mocking out WarehouseRepository
*/
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <modules/SulDownloader/ConfigurationData.h>
#include <modules/Common/UtilityImpl/MessageUtility.h>


#include "tests/Common/TestHelpers/TempDir.h"
#include "MockWarehouseRepository.h"
#include "ConfigurationSettings.pb.h"
#include "SulDownloader/SulDownloader.h"
#include "Common/ProcessImpl/ArgcAndEnv.h"
#include "TestWarehouseHelper.h"
#include "SulDownloader/SulDownloaderException.h"
#include "SulDownloader/DownloadReport.h"
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
        m_tempDir = Tests::TempDir::makeTempDir();
        m_tempDir->makeDirs(std::vector<std::string>{"update/certificates",
                             "update/cache/PrimaryWarehouse",
                             "update/cache/Primary",
                             "etc/ssl",
                             "etc/cache/ssl"});
        m_tempDir->createFile("update/certificates/ps_rootca.crt", "empty");
        m_tempDir->createFile("update/certificates/rootca.crt", "empty");
        m_mockptr = nullptr;
    }

    /**
     * Remove the temporary directory.
     */
    void TearDown() override
    {
        TestWarehouseHelper helper;
        helper.restoreWarehouseFactory();
        m_tempDir.reset(nullptr);
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

        settings.set_certificatepath(m_tempDir->absPath("update/certificates"));
        settings.set_installationrootpath(m_tempDir->dirPath());
        settings.set_systemsslpath(m_tempDir->absPath("etc/ssl"));
        settings.set_cacheupdatesslpath(m_tempDir->absPath("etc/cache/ssl"));

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
            product.setDistributePath(m_tempDir->absPath(std::string("update/cache/Primary/")));
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



protected:
    std::unique_ptr<Tests::TempDir> m_tempDir ;
    MockWarehouseRepository * m_mockptr = nullptr;
};

TEST_F( SULDownloaderTest, configurationDataVerificationOfDefaultSettingsReturnsTrue )
{
    SulDownloader::ConfigurationData confData = configData( defaultSettings());
    confData.verifySettingsAreValid();
    EXPECT_TRUE( confData.isVerified());
}

TEST_F( SULDownloaderTest, main_entry_InvalidArgumentsReturnsTheCorrectErrorCode)
{
    int expectedErrorCode = -2;

    char ** argsNotUsed = nullptr;
    EXPECT_EQ( SulDownloader::main_entry(2, argsNotUsed), -1);
    EXPECT_EQ( SulDownloader::main_entry(1, argsNotUsed), -1);
    char * inputFileDoesNotExist [] = {const_cast<char*>("SulDownloader"),
                                       const_cast<char*>("inputfiledoesnotexists.json"),
                                       const_cast<char*>("createoutputpath.json")};
    EXPECT_EQ( SulDownloader::main_entry(3, inputFileDoesNotExist), -2);

    m_tempDir->createFile("input.json", jsonSettings(defaultSettings()));
    // directory can not be replaced by file
    Common::ProcessImpl::ArgcAndEnv args("SulDownloader", {m_tempDir->absPath("input.json"), m_tempDir->absPath("etc")}, {});

    EXPECT_EQ( SulDownloader::main_entry(3, args.argc()), expectedErrorCode);
}

/**
 * Disabled due to LINUXEP-6174, re-enable this test once that ticket has been fixed.
 * Breaks when build dir and /tmp are on different partitions.
 */
TEST_F( SULDownloaderTest, DISABLED_main_entry_onSuccessCreatesReportContainingExpectedSuccessResult)
{
    MockWarehouseRepository & mock = warehouseMocked();

    auto products = defaultProducts();
    products[0].setProductHasChanged(false);
    products[1].setProductHasChanged(false);

    EXPECT_CALL(mock, hasError()).WillRepeatedly(Return(false));
    EXPECT_CALL(mock, synchronize(_));
    EXPECT_CALL(mock, distribute());
    EXPECT_CALL(mock, getProducts()).WillOnce(Return(products));



    m_tempDir->createFile("input.json", jsonSettings(defaultSettings()));
    Common::ProcessImpl::ArgcAndEnv args("SulDownloader", {m_tempDir->absPath("input.json"), m_tempDir->absPath("output.json")}, {});

    EXPECT_EQ( SulDownloader::main_entry(3, args.argc()), 0);

    // read the file content
    std::string content = m_tempDir->fileContent("output.json");
    EXPECT_THAT( content, ::testing::HasSubstr( SulDownloader::toString(SulDownloader::WarehouseStatus::SUCCESS)));

}

// the other execution paths were covered in main_entry_* tests.
TEST_F( SULDownloaderTest, fileEntriesAndRunDownloaderThrowIfCannotCreateOutputFile)
{
    m_tempDir->createFile("input.json", jsonSettings(defaultSettings()));

    EXPECT_THROW(SulDownloader::fileEntriesAndRunDownloader(
            m_tempDir->absPath("input.json"), m_tempDir->absPath("path/that/cannot/be/created/output.json")),
            SulDownloader::SulDownloaderException);
}

// configAndRunDownloader
TEST_F( SULDownloaderTest, configAndRunDownloaderInvalidSettingsReportError_WarehouseStatus_UNSPECIFIED)
{
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

//TEST_F( SULDownloaderTest, configAndRunDownloaderInvalidSettingsReportErrorAndLogExpectedErrorInformation)
//{
//    std::string reportContent;
//    int exitCode =0;
//    auto settings = defaultSettings();
//    settings.clear_sophosurls(); // no sophos urls, the system can not connect to warehouses
//    std::string settingsString = jsonSettings(settings);
//
//    std::tie(exitCode,reportContent) = SulDownloader::configAndRunDownloader(settingsString);
//
//    EXPECT_NE(exitCode, 0 );
//    EXPECT_THAT( reportContent, ::testing::Not(::testing::HasSubstr( SulDownloader::toString(SulDownloader::WarehouseStatus::SUCCESS))));
//    EXPECT_THAT( reportContent, ::testing::HasSubstr( SulDownloader::toString(SulDownloader::WarehouseStatus::UNSPECIFIED)));
//    //Add additional checks for log details.
//}


// runSULDownloader
TEST_F( SULDownloaderTest, runSULDownloader_WarehouseConnectionFailureShouldCreateValidConnectionFailureReport)
{
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
 * Simulate error in installing base successfully but fail to install plugin
 */
TEST_F( SULDownloaderTest, runSULDownloader_PluginInstallationFailureShouldResultInValidInstalledFailedReport)
{
    MockWarehouseRepository & mock = warehouseMocked();

    std::vector<SulDownloader::DownloadedProduct> products = defaultProducts();

    for ( auto & product : products)
    {
        product.setProductHasChanged(true);
    }

    m_tempDir->createFile("update/cache/Primary/everest/install.sh", R"(#! /bin/bash
echo "installing base"
exit 0; )");
    m_tempDir->createFile("update/cache/Primary/everest-plugin-a/install.sh", R"(#! /bin/bash
echo "installing plugin"
echo "simulate failure"
exit 5; )");
    m_tempDir->makeExecutable("update/cache/Primary/everest/install.sh" );
    m_tempDir->makeExecutable("update/cache/Primary/everest-plugin-a/install.sh" );


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
    MockWarehouseRepository & mock = warehouseMocked();

    std::vector<SulDownloader::DownloadedProduct> products = defaultProducts();

    for ( auto & product : products)
    {
        product.setProductHasChanged(true);
    }

    m_tempDir->createFile("update/cache/Primary/everest/install.sh", R"(#! /bin/bash
echo "installing base"
exit 0; )");
    m_tempDir->createFile("update/cache/Primary/everest-plugin-a/install.sh", R"(#! /bin/bash
echo "installing plugin"
exit 0; )");
    m_tempDir->makeExecutable("update/cache/Primary/everest/install.sh" );
    m_tempDir->makeExecutable("update/cache/Primary/everest-plugin-a/install.sh" );


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


