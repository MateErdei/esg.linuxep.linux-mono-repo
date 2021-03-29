/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "../common/LogInitializedTests.h"

// Include the .cpp to get static functions...
#include "sophos_threat_detector/threat_scanner/SusiWrapperFactory.cpp" // NOLINT

#include <gtest/gtest.h>

#define BASE "/tmp/TestSusiWrapperFactory/chroot/"
using namespace threat_scanner;

namespace
{
    class TestSusiWrapperFactory : public LogInitializedTests
    {
    public:

        void setupFilesForTestingGlobalRep();
        void setupEmptySusiStartupFile();
        void writeBinaryToSusiStartupFile();
        void writeLargeSusiStartupFile();
        void writeToSusiStartupFile(const std::string& susiStartupContents);

        fs::path getSusiStartupPath() { return m_susiStartupPath; }
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
            m_machineIdpath = m_fakeEtcPath;
            m_machineIdpath /= "machine_id.txt";

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

        fs::path m_machineIdpath;
        fs::path m_customerIdPath;
        fs::path m_susiStartupPath;
    };
}

void TestSusiWrapperFactory::setupFilesForTestingGlobalRep()
{
    createTestFolderAndSetupPaths();

    std::ofstream machineIdFile(m_machineIdpath);
    ASSERT_TRUE(machineIdFile.good());
    machineIdFile << "ab7b6758a3ab11ba8a51d25aa06d1cf4";
    machineIdFile.close();

    std::ofstream customerIdFileStream(m_customerIdPath);
    ASSERT_TRUE(customerIdFileStream.good());
    customerIdFileStream << "d22829d94b76c016ec4e04b08baeffaa";
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

TEST_F(TestSusiWrapperFactory, getCustomerIdReturnsId) // NOLINT
{
    setupFilesForTestingGlobalRep();
    EXPECT_EQ(getCustomerId(),"d22829d94b76c016ec4e04b08baeffaa");
}

TEST_F(TestSusiWrapperFactory, isSxlLookupEnabledReturnsFalse) // NOLINT
{
    writeToSusiStartupFile(R"sophos({"enableSxlLookup":false})sophos");
    EXPECT_FALSE(isSxlLookupEnabled());
}

TEST_F(TestSusiWrapperFactory, isSxlLookupEnabledReturnsTrue) // NOLINT
{
    writeToSusiStartupFile(R"sophos({"enableSxlLookup":true})sophos");
    EXPECT_TRUE(isSxlLookupEnabled());
}

TEST_F(TestSusiWrapperFactory, isSxlLookupEnabled_doesNotExist) // NOLINT
{
    EXPECT_TRUE(isSxlLookupEnabled());
}

TEST_F(TestSusiWrapperFactory, isSxlLookupEnabled_emptyFile)
{
    setupEmptySusiStartupFile();
    EXPECT_TRUE(isSxlLookupEnabled());
}

TEST_F(TestSusiWrapperFactory, isSxlLookupEnabled_invalidJson)
{
    writeToSusiStartupFile(R"sophos({"enableSxlLookup":false)sophos");
    EXPECT_TRUE(isSxlLookupEnabled());
}

TEST_F(TestSusiWrapperFactory, isSxlLookupEnabled_unexpectedValue)
{
    writeToSusiStartupFile(R"sophos({"enableSxlLookup":"false"})sophos");
    EXPECT_TRUE(isSxlLookupEnabled());
}

TEST_F(TestSusiWrapperFactory, isSxlLookupEnabled_multipleValuesEqual)
{
    writeToSusiStartupFile(R"sophos({"enableSxlLookup":false, "enableSxlLookup":false})sophos");
    EXPECT_FALSE(isSxlLookupEnabled());
}

TEST_F(TestSusiWrapperFactory, isSxlLookupEnabled_multipleValuesFalseLast)
{
    writeToSusiStartupFile(R"sophos({"enableSxlLookup":true, "enableSxlLookup":false})sophos");
    EXPECT_FALSE(isSxlLookupEnabled());
}

TEST_F(TestSusiWrapperFactory, isSxlLookupEnabled_multipleValuesTrueLast)
{
    writeToSusiStartupFile(R"sophos({"enableSxlLookup":false, "enableSxlLookup":true})sophos");
    EXPECT_TRUE(isSxlLookupEnabled());
}

TEST_F(TestSusiWrapperFactory, isSxlLookupEnabled_manyMultipleValuesTrueLast)
{
    writeToSusiStartupFile(R"sophos({"enableSxlLookup":false, "enableSxlLookup":true, "enableSxlLookup":true, "enableSxlLookup":false, "enableSxlLookup":true})sophos");
    EXPECT_TRUE(isSxlLookupEnabled());
}

TEST_F(TestSusiWrapperFactory, isSxlLookupEnabled_array)
{
    writeToSusiStartupFile(R"sophos({"enableSxlLookup":[false]})sophos");
    EXPECT_TRUE(isSxlLookupEnabled());
}

TEST_F(TestSusiWrapperFactory, isSxlLookupEnabled_binaryFile)
{
    writeBinaryToSusiStartupFile();
    EXPECT_TRUE(isSxlLookupEnabled());
}

TEST_F(TestSusiWrapperFactory, isSxlLookupEnabled_wrongPermissions)
{
    writeToSusiStartupFile(R"sophos({"enableSxlLookup":false})sophos");
    fs::permissions(getSusiStartupPath(), fs::perms::remove_perms | fs::perms::all);
    EXPECT_TRUE(isSxlLookupEnabled());
}

TEST_F(TestSusiWrapperFactory, isSxlLookupEnabled_veryLargeFile)
{
    writeLargeSusiStartupFile();
    EXPECT_FALSE(isSxlLookupEnabled());
}
