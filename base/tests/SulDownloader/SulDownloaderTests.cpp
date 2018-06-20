///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
/**
* Component tests to SULDownloader mocking out WarehouseRepository
*/
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <modules/SulDownloader/ConfigurationData.h>
#include <modules/SulDownloader/MessageUtility.h>


#include "tests/Common/TestHelpers/TempDir.h"
#include "Common/FileSystemImpl/FileSystemImpl.h"
#include "MockWarehouseRepository.h"
#include "ConfigurationSettings.pb.h"
#include "SulDownloader/Credentials.h"
#include "SulDownloader/ConfigurationData.h"
#include "SulDownloader/SulDownloader.h"
#include "Common/ProcessImpl/ArgcAndEnv.h"
#include "TestWarehouseHelper.h"
#include "SulDownloader/SulDownloaderException.h"
using SulDownloaderProto::ConfigurationSettings;

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
        return SulDownloader::MessageUtility::protoBuf2Json( configSettings );
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
            product.setDistributePath(m_tempDir->absPath(std::string("update/cache/Primary/") + metadata.getDefaultHomePath()));
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
        plugin.setDefaultHomePath("everest-plugin");
        plugin.setLine("Everest-Plugins-A");
        plugin.setName("Everest-Plugins-A-Product");
        plugin.setVersion("10.3.5");
        plugin.setTags({{"RECOMMENDED","10", "Plugin-label"}});

        return std::vector<SulDownloader::ProductMetadata>{base,plugin};


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




protected:
    std::unique_ptr<Tests::TempDir> m_tempDir ;
    MockWarehouseRepository * m_mockptr = nullptr;
};

TEST_F( SULDownloaderTest, defaultSettingsIsValid )
{
    SulDownloader::ConfigurationData confData = configData( defaultSettings());
    confData.verifySettingsAreValid();
    EXPECT_TRUE( confData.isVerified());
}

TEST_F( SULDownloaderTest, main_entry_InvalidArgumentsQuitExecution)
{
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
    EXPECT_EQ( SulDownloader::main_entry(3, args.argc()), -2);
}

TEST_F( SULDownloaderTest, main_entry_onSuccessCreatesReport)
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
TEST_F( SULDownloaderTest, configAndRunDownloaderInvalidSettingsReportError)
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
    //FIXME: the log will contain the reason for the settings failure. Add this when log is available.
}