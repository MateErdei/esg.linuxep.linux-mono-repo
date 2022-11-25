/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "../../common/LogInitializedTests.h"

// Include the .cpp to get static functions...
#include "sophos_threat_detector/threat_scanner/SusiWrapperFactory.cpp" // NOLINT

#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/MockFileSystem.h"

#include <gtest/gtest.h>

using namespace threat_scanner;

namespace
{
    class TestSusiWrapperFactory : public LogInitializedTests
    {
    public:

        void setupFilesForTestingGlobalRep(const std::string& customerId, const std::string& machineId);
        void setupEmptySusiStartupFile();
        void writeBinaryToSusiStartupFile();
        void writeLargeSusiStartupFile();
        void writeToSusiStartupFile(const std::string& susiStartupContents);

    protected:
        void SetUp() override
        {
            const ::testing::TestInfo* const test_info = ::testing::UnitTest::GetInstance()->current_test_info();
            m_testDir = fs::temp_directory_path();
            m_testDir /= test_info->test_case_name();
            m_testDir /= test_info->name();
            fs::remove_all(m_testDir);
            fs::create_directories(m_testDir);
            fs::current_path(m_testDir);
        }

        void TearDown() override
        {
            fs::current_path(fs::temp_directory_path());
            fs::remove_all(m_testDir);
        }

        void createTestFolderAndSetupPaths()
        {
            auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
            appConfig.setData("SOPHOS_INSTALL", m_testDir);
            PLUGIN_INSTALL = m_testDir;
            PLUGIN_INSTALL /= "plugins/av";
            appConfig.setData("PLUGIN_INSTALL", PLUGIN_INSTALL);

            fs::path m_fakeEtcPath  = m_testDir;
            m_fakeEtcPath  /= "base/etc";
            m_machineIdPath = m_fakeEtcPath;
            m_machineIdPath /= "machine_id.txt";

            m_customerIdPath = PLUGIN_INSTALL;
            m_customerIdPath /= "var/customer_id.txt";
            auto varDirectory = m_customerIdPath.parent_path();

            m_susiStartupPath = PLUGIN_INSTALL;
            m_susiStartupPath /= "var/susi_startup_settings.json";

            fs::create_directories(m_fakeEtcPath);
            fs::create_directories(varDirectory);
        }

        fs::path PLUGIN_INSTALL;
        fs::path m_testDir;

        fs::path m_machineIdPath;
        fs::path m_customerIdPath;
        fs::path m_susiStartupPath;
    };
}

void TestSusiWrapperFactory::setupFilesForTestingGlobalRep(const std::string& customerId="d22829d94b76c016ec4e04b08baeffaa", const std::string& machineId="ab7b6758a3ab11ba8a51d25aa06d1cf4")
{
    createTestFolderAndSetupPaths();

    std::ofstream machineIdFile(m_machineIdPath);
    ASSERT_TRUE(machineIdFile.good());
    machineIdFile << machineId;
    machineIdFile.close();

    std::ofstream customerIdFileStream(m_customerIdPath);
    ASSERT_TRUE(customerIdFileStream.good());
    customerIdFileStream << customerId;
    customerIdFileStream.close();
}

void TestSusiWrapperFactory::setupEmptySusiStartupFile()
{
    writeToSusiStartupFile("");
}

void TestSusiWrapperFactory::writeToSusiStartupFile(const std::string& susiStartupContents)
{
    createTestFolderAndSetupPaths();

    std::ofstream susiSettingsFileStream(m_susiStartupPath);
    ASSERT_TRUE(susiSettingsFileStream.good());
    susiSettingsFileStream << susiStartupContents;
    susiSettingsFileStream.close();
}

void TestSusiWrapperFactory::writeBinaryToSusiStartupFile()
{
    createTestFolderAndSetupPaths();
    char buffer[100] = {0};
    std::ofstream susiSettingsFileStream(m_susiStartupPath, std::ios::out | std::ios::binary);
    ASSERT_TRUE(susiSettingsFileStream.good());
    susiSettingsFileStream.write(buffer, 100);
    susiSettingsFileStream.close();
}

void TestSusiWrapperFactory::writeLargeSusiStartupFile()
{
    std::ostringstream largeContents;
    largeContents << "{\"enableSxlLookup\":false, ";

    for (int i = 0; i < 100000; i++)
    {
        largeContents << "\"fakeEntry" << i << "\": \"" << i << "\", ";
    }
    largeContents << R"("finalFakeEntry": "last"})";
    writeToSusiStartupFile(largeContents.str());
}

TEST_F(TestSusiWrapperFactory, constructAndShutdown)
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    appConfig.setData("PLUGIN_INSTALL", "/tmp/TestSusiWrapperFactory");
    SusiWrapperFactory wrapperFact{};
    wrapperFact.shutdown();
}

TEST_F(TestSusiWrapperFactory, getCustomerIdReturnsUnknown) // NOLINT
{
    EXPECT_EQ(getCustomerId(),"c1cfcf69a42311a6084bcefe8af02c8a");
}

TEST_F(TestSusiWrapperFactory, getEndpointIdReturnsUnknown) // NOLINT
{
    EXPECT_EQ(getEndpointId(),"66b8fd8b39754951b87269afdfcb285c");
}

TEST_F(TestSusiWrapperFactory, getEndpointIdReturnsId) // NOLINT
{
    setupFilesForTestingGlobalRep();
    EXPECT_EQ(getEndpointId(),"ab7b6758a3ab11ba8a51d25aa06d1cf4");
}

TEST_F(TestSusiWrapperFactory, getEndpointIdWithEmptyFile) // NOLINT
{
    setupFilesForTestingGlobalRep("","");
    EXPECT_EQ(getEndpointId(),"66b8fd8b39754951b87269afdfcb285c");
}

TEST_F(TestSusiWrapperFactory, getEndpointIdWithNewLine) // NOLINT
{
    setupFilesForTestingGlobalRep("","c1cfcf69a42311a\n084bcefe8af02c8a");
    EXPECT_EQ(getEndpointId(),"66b8fd8b39754951b87269afdfcb285c");
}

TEST_F(TestSusiWrapperFactory, getEndpointIdWithEmptySpace) // NOLINT
{
    setupFilesForTestingGlobalRep("","ab7b6758a3ab1 ba8a51d25aa06d1cf4");
    EXPECT_EQ(getEndpointId(),"66b8fd8b39754951b87269afdfcb285c");
}

TEST_F(TestSusiWrapperFactory, getEndpointIdOfWrongSize) // NOLINT
{
    setupFilesForTestingGlobalRep("","ab7b6758a3ab11ba8a51d25aa06d1cf4 \n d22829d94b76c016ec4e04b08baeffaa");
    EXPECT_EQ(getEndpointId(),"66b8fd8b39754951b87269afdfcb285c");

    setupFilesForTestingGlobalRep("","ab7b6758a3ab11ba8a51d25aa06d1cf4ab7b6758a3ab11ba8a51d25aa06d1cf4");
    EXPECT_EQ(getEndpointId(),"66b8fd8b39754951b87269afdfcb285c");

    setupFilesForTestingGlobalRep("","a");
    EXPECT_EQ(getEndpointId(),"66b8fd8b39754951b87269afdfcb285c");
}

TEST_F(TestSusiWrapperFactory, getEndpointIdWithNonHex) // NOLINT
{
    setupFilesForTestingGlobalRep("","GgGgGgGgGgGgGgGgGgGgGgGgGgGgGgGg");
    EXPECT_EQ(getEndpointId(),"66b8fd8b39754951b87269afdfcb285c");
}

TEST_F(TestSusiWrapperFactory, getEndpointIdWithNonUTF8)
{
    // echo -n "名前の付いたオンデマンド検索の設定" | iconv -f utf-8 -t euc-jp | hexdump -C
    std::vector<unsigned char> threatPathBytes { 0xcc, 0xbe, 0xc1, 0xb0, 0xa4, 0xce, 0xc9, 0xd5, 0xa4, 0xa4, 0xa4, 0xbf,
                                                 0xa5, 0xaa, 0xa5, 0xf3, 0xa5, 0xc7, 0xa5, 0xde, 0xa5, 0xf3, 0xa5, 0xc9,
                                                 0xb8, 0xa1, 0xba, 0xf7, 0xa4, 0xce, 0xc0, 0xc0};
    std::string nonUTF8EndpointId(threatPathBytes.begin(), threatPathBytes.end());

    setupFilesForTestingGlobalRep("",nonUTF8EndpointId);
    EXPECT_EQ(getEndpointId(),"66b8fd8b39754951b87269afdfcb285c");
}

TEST_F(TestSusiWrapperFactory, getCustomerIdReturnsId) // NOLINT
{
    setupFilesForTestingGlobalRep();
    EXPECT_EQ(getCustomerId(),"d22829d94b76c016ec4e04b08baeffaa");
}

TEST_F(TestSusiWrapperFactory, getCustomerIdWithEmptyFile) // NOLINT
{
    setupFilesForTestingGlobalRep("");
    EXPECT_EQ(getCustomerId(),"c1cfcf69a42311a6084bcefe8af02c8a");
}

TEST_F(TestSusiWrapperFactory, getCustomerIdWithNewLine) // NOLINT
{
    setupFilesForTestingGlobalRep("c1cfcf69a42311a\n084bcefe8af02c8a");
    EXPECT_EQ(getCustomerId(),"c1cfcf69a42311a6084bcefe8af02c8a");
}


TEST_F(TestSusiWrapperFactory, getCustomerIdWithEmptySpace) // NOLINT
{
    setupFilesForTestingGlobalRep("d22829d94b76c 16ec4e04b08baeffaa");
    EXPECT_EQ(getCustomerId(),"c1cfcf69a42311a6084bcefe8af02c8a");
}

TEST_F(TestSusiWrapperFactory, getCustomerIdOfWrongSize) // NOLINT
{
    setupFilesForTestingGlobalRep("d22829d94b76c016ec4e04b08baeffaa \n d22829d94b76c016ec4e04b08baeffaa");
    EXPECT_EQ(getCustomerId(),"c1cfcf69a42311a6084bcefe8af02c8a");

    setupFilesForTestingGlobalRep("d22829d94b76c016ec4e04b08baeffaad22829d94b76c016ec4e04b08baeffaa");
    EXPECT_EQ(getCustomerId(),"c1cfcf69a42311a6084bcefe8af02c8a");

    setupFilesForTestingGlobalRep("a");
    EXPECT_EQ(getCustomerId(),"c1cfcf69a42311a6084bcefe8af02c8a");
}

TEST_F(TestSusiWrapperFactory, getCustomerIdWithNonHex) // NOLINT
{
    setupFilesForTestingGlobalRep("GgGgGgGgGgGgGgGgGgGgGgGgGgGgGgGg");
    EXPECT_EQ(getCustomerId(),"c1cfcf69a42311a6084bcefe8af02c8a");
}

TEST_F(TestSusiWrapperFactory, getCustomerIdWithNonUTF8)
{
    // echo -n "名前の付いたオンデマンド検索の設定" | iconv -f utf-8 -t euc-jp | hexdump -C
    std::vector<unsigned char> threatPathBytes { 0xcc, 0xbe, 0xc1, 0xb0, 0xa4, 0xce, 0xc9, 0xd5, 0xa4, 0xa4, 0xa4, 0xbf,
                                                 0xa5, 0xaa, 0xa5, 0xf3, 0xa5, 0xc7, 0xa5, 0xde, 0xa5, 0xf3, 0xa5, 0xc9,
                                                 0xb8, 0xa1, 0xba, 0xf7, 0xa4, 0xce, 0xc0, 0xc0};
    std::string nonUTF8CustomerId(threatPathBytes.begin(), threatPathBytes.end());

    setupFilesForTestingGlobalRep(nonUTF8CustomerId);
    EXPECT_EQ(getCustomerId(),"c1cfcf69a42311a6084bcefe8af02c8a");
}

TEST_F(TestSusiWrapperFactory, isSxlLookupEnabledReturnsFalse) // NOLINT
{
    writeToSusiStartupFile(R"sophos({"enableSxlLookup":false})sophos");
    common::ThreatDetector::SusiSettings threatDetectorSettings;
    threatDetectorSettings.load(m_susiStartupPath);
    EXPECT_FALSE(threatDetectorSettings.isSxlLookupEnabled());
}

TEST_F(TestSusiWrapperFactory, isSxlLookupEnabledReturnsTrue) // NOLINT
{
    writeToSusiStartupFile(R"sophos({"enableSxlLookup":true})sophos");
    common::ThreatDetector::SusiSettings threatDetectorSettings;
    threatDetectorSettings.load(m_susiStartupPath);
    EXPECT_TRUE(threatDetectorSettings.isSxlLookupEnabled());
}

TEST_F(TestSusiWrapperFactory, isSxlLookupEnabled_doesNotExist) // NOLINT
{
    common::ThreatDetector::SusiSettings threatDetectorSettings;
    threatDetectorSettings.load(m_susiStartupPath);
    EXPECT_TRUE(threatDetectorSettings.isSxlLookupEnabled());
}

TEST_F(TestSusiWrapperFactory, isSxlLookupEnabled_emptyFile)
{
    setupEmptySusiStartupFile();
    common::ThreatDetector::SusiSettings threatDetectorSettings;
    threatDetectorSettings.load(m_susiStartupPath);
    EXPECT_TRUE(threatDetectorSettings.isSxlLookupEnabled());
}

TEST_F(TestSusiWrapperFactory, isSxlLookupEnabled_invalidJson)
{
    writeToSusiStartupFile(R"sophos({"enableSxlLookup":false)sophos");
    common::ThreatDetector::SusiSettings threatDetectorSettings;
    threatDetectorSettings.load(m_susiStartupPath);
    EXPECT_TRUE(threatDetectorSettings.isSxlLookupEnabled());
}

TEST_F(TestSusiWrapperFactory, isSxlLookupEnabled_string)
{
    writeToSusiStartupFile(R"sophos({"enableSxlLookup":"false"})sophos");
    common::ThreatDetector::SusiSettings threatDetectorSettings;
    threatDetectorSettings.load(m_susiStartupPath);
    EXPECT_TRUE(threatDetectorSettings.isSxlLookupEnabled());
}

TEST_F(TestSusiWrapperFactory, isSxlLookupEnabled_unexpectedValue)
{
    writeToSusiStartupFile(R"sophos({"enableSxlLookup":"none"})sophos");
    common::ThreatDetector::SusiSettings threatDetectorSettings;
    threatDetectorSettings.load(m_susiStartupPath);
    EXPECT_TRUE(threatDetectorSettings.isSxlLookupEnabled());
}

TEST_F(TestSusiWrapperFactory, isSxlLookupEnabled_corruptedValue)
{
    writeToSusiStartupFile(R"sophos({"enableSxlLookup":fals})sophos");
    common::ThreatDetector::SusiSettings threatDetectorSettings;
    threatDetectorSettings.load(m_susiStartupPath);
    EXPECT_TRUE(threatDetectorSettings.isSxlLookupEnabled());
}

TEST_F(TestSusiWrapperFactory, isSxlLookupEnabled_corruptedKey)
{
    writeToSusiStartupFile(R"sophos({"enableSxlLooku":false})sophos");
    common::ThreatDetector::SusiSettings threatDetectorSettings;
    threatDetectorSettings.load(m_susiStartupPath);
    EXPECT_TRUE(threatDetectorSettings.isSxlLookupEnabled());
}

TEST_F(TestSusiWrapperFactory, isSxlLookupEnabled_multipleValuesEqual)
{
    writeToSusiStartupFile(R"sophos({"enableSxlLookup":false, "enableSxlLookup":false})sophos");
    common::ThreatDetector::SusiSettings threatDetectorSettings;
    threatDetectorSettings.load(m_susiStartupPath);
    EXPECT_FALSE(threatDetectorSettings.isSxlLookupEnabled());
}

TEST_F(TestSusiWrapperFactory, isSxlLookupEnabled_multipleValuesFalseLast)
{
    writeToSusiStartupFile(R"sophos({"enableSxlLookup":true, "enableSxlLookup":false})sophos");
    common::ThreatDetector::SusiSettings threatDetectorSettings;
    threatDetectorSettings.load(m_susiStartupPath);
    EXPECT_FALSE(threatDetectorSettings.isSxlLookupEnabled());
}

TEST_F(TestSusiWrapperFactory, isSxlLookupEnabled_multipleValuesTrueLast)
{
    writeToSusiStartupFile(R"sophos({"enableSxlLookup":false, "enableSxlLookup":true})sophos");
    common::ThreatDetector::SusiSettings threatDetectorSettings;
    threatDetectorSettings.load(m_susiStartupPath);
    EXPECT_TRUE(threatDetectorSettings.isSxlLookupEnabled());
}

TEST_F(TestSusiWrapperFactory, isSxlLookupEnabled_manyMultipleValuesTrueLast)
{
    writeToSusiStartupFile(R"sophos({"enableSxlLookup":false, "enableSxlLookup":true, "enableSxlLookup":true, "enableSxlLookup":false, "enableSxlLookup":true})sophos");
    common::ThreatDetector::SusiSettings threatDetectorSettings;
    threatDetectorSettings.load(m_susiStartupPath);
    EXPECT_TRUE(threatDetectorSettings.isSxlLookupEnabled());
}

TEST_F(TestSusiWrapperFactory, isSxlLookupEnabled_array)
{
    writeToSusiStartupFile(R"sophos({"enableSxlLookup":[false]})sophos");
    common::ThreatDetector::SusiSettings threatDetectorSettings;
    threatDetectorSettings.load(m_susiStartupPath);
    EXPECT_TRUE(threatDetectorSettings.isSxlLookupEnabled());
}

TEST_F(TestSusiWrapperFactory, isSxlLookupEnabled_binaryFile)
{
    writeBinaryToSusiStartupFile();
    common::ThreatDetector::SusiSettings threatDetectorSettings;
    threatDetectorSettings.load(m_susiStartupPath);
    EXPECT_TRUE(threatDetectorSettings.isSxlLookupEnabled());
}

TEST_F(TestSusiWrapperFactory, isSxlLookupEnabled_veryLargeFile)
{
    writeLargeSusiStartupFile();
    common::ThreatDetector::SusiSettings threatDetectorSettings;
    threatDetectorSettings.load(m_susiStartupPath);
    EXPECT_FALSE(threatDetectorSettings.isSxlLookupEnabled());
}

TEST_F(TestSusiWrapperFactory, susiWrapperFactoryUpdateSusiConfigRegistersChange)
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    appConfig.setData("PLUGIN_INSTALL", "/tmp/TestSusiWrapperFactory");
    auto filesystemMock = new StrictMock<MockFileSystem>();

    std::string json1 = R"({"enableSxlLookup":true,"shaAllowList":["42268ef08462e645678ce738bd26518bc170a0404a186062e8b1bec2dc578673"]})";
    std::string json2 = R"({"enableSxlLookup":false,"shaAllowList":["42268ef08462e645678ce738bd26518bc170a0404a186062e8b1bec2dc578673"]})";
    std::string json3 = R"({"enableSxlLookup":false,"shaAllowList":["different"]})";

    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSusiStartupSettingsPath())).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getSusiStartupSettingsPath()))
        .WillOnce(Return(json1)) // Load this one at construction
        .WillOnce(Return(json2)) // On update call, load this one and return changed
        .WillOnce(Return(json3)) // On update call, load this one and return changed
        .WillOnce(Return(json3)); // On update call, load this one again and return unchanged

    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    SusiWrapperFactory susiWrapperFactory{};
    ASSERT_TRUE(susiWrapperFactory.updateSusiConfig());
    ASSERT_TRUE(susiWrapperFactory.updateSusiConfig());
    ASSERT_FALSE(susiWrapperFactory.updateSusiConfig());
}