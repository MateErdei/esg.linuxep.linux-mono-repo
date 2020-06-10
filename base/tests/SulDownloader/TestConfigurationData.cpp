/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ConfigurationDataBase.h"
#include <Common/Logging/ConsoleLoggingSetup.h>

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystemImpl/FileSystemImpl.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <SulDownloader/suldownloaderdata/ConfigurationData.h>
#include <SulDownloader/suldownloaderdata/SulDownloaderException.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/MockFileSystem.h>
using namespace SulDownloader;
using namespace SulDownloader::suldownloaderdata;

class ConfigurationDataTest : public ConfigurationDataBase
{
public:
    ~ConfigurationDataTest() { Tests::restoreFileSystem(); }

    MockFileSystem& setupFileSystemAndGetMock()
    {
        using ::testing::Ne;
        Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL, "/installroot");

        auto filesystemMock = new NiceMock<MockFileSystem>();
        ON_CALL(*filesystemMock, isDirectory(m_absInstallationPath)).WillByDefault(Return(true));
        ON_CALL(*filesystemMock, isDirectory(m_primaryPath)).WillByDefault(Return(true));
        ON_CALL(*filesystemMock, isDirectory(m_distPath)).WillByDefault(Return(true));
        ON_CALL(*filesystemMock, isDirectory(m_absCertificatePath)).WillByDefault(Return(true));
        std::string empty;
        ON_CALL(*filesystemMock, exists(empty)).WillByDefault(Return(false));
        ON_CALL(*filesystemMock, exists(Ne(empty))).WillByDefault(Return(true));

        auto pointer = filesystemMock;
        Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));
        return *pointer;
    }

    ::testing::AssertionResult configurationDataIsEquivalent(
        const char* m_expr,
        const char* n_expr,
        const SulDownloader::suldownloaderdata::ConfigurationData& expected,
        const SulDownloader::suldownloaderdata::ConfigurationData& resulted)
    {
        std::stringstream s;
        s << m_expr << " and " << n_expr << " failed: ";

        if (expected.getLogLevel() != resulted.getLogLevel())
        {
            return ::testing::AssertionFailure() << s.str() << "log level differs";
        }

        if (expected.getUpdateCacheSslCertificatePath() != resulted.getUpdateCacheSslCertificatePath())
        {
            return ::testing::AssertionFailure() << s.str() << "update cache certificate path differs";
        }

        if (expected.getCredentials() != resulted.getCredentials())
        {
            return ::testing::AssertionFailure() << s.str() << "credentials differ";
        }

        if (expected.getSophosUpdateUrls() != resulted.getSophosUpdateUrls())
        {
            return ::testing::AssertionFailure() << s.str() << "update urls differ";
        }

        if (expected.getLocalUpdateCacheUrls() != resulted.getLocalUpdateCacheUrls())
        {
            return ::testing::AssertionFailure() << s.str() << "update cache urls differ";
        }

        if (expected.getPolicyProxy().getUrl() != resulted.getPolicyProxy().getUrl())
        {
            return ::testing::AssertionFailure()
                   << s.str() << "proxy urls differ " << expected.getPolicyProxy().getUrl()
                   << " != " << resulted.getPolicyProxy().getUrl();
        }

        if (expected.getPolicyProxy() != resulted.getPolicyProxy())
        {
            return ::testing::AssertionFailure() << s.str() << "proxy credentials differ ";
        }

        if (expected.proxiesList() != resulted.proxiesList())
        {
            return ::testing::AssertionFailure() << s.str() << "proxy list differs";
        }

        if (expected.getInstallationRootPath() != resulted.getInstallationRootPath())
        {
            return ::testing::AssertionFailure() << s.str() << "installation root path differs";
        }

        if (expected.getLocalWarehouseRepository() != resulted.getLocalWarehouseRepository())
        {
            return ::testing::AssertionFailure() << s.str() << "local warehouse repository differs";
        }

        if (expected.getLocalDistributionRepository() != resulted.getLocalDistributionRepository())
        {
            return ::testing::AssertionFailure() << s.str() << "local distribution repository differs";
        }

        if (expected.getCertificatePath() != resulted.getCertificatePath())
        {
            return ::testing::AssertionFailure() << s.str() << "certificate path differs";
        }

        if (expected.getSystemSslCertificatePath() != resulted.getSystemSslCertificatePath())
        {
            return ::testing::AssertionFailure() << s.str() << "system ssl certificate path differs";
        }

        if (expected.getInstallArguments() != resulted.getInstallArguments())
        {
            return ::testing::AssertionFailure() << s.str() << "install arguments differs";
        }

        if (expected.getPrimarySubscription() != resulted.getPrimarySubscription())
        {
            return ::testing::AssertionFailure() << s.str() << "primary subscription differs";
        }

        if (expected.getProductsSubscription() != resulted.getProductsSubscription())
        {
            return ::testing::AssertionFailure() << s.str() << "products subscription differs";
        }

        if (expected.getFeatures() != resulted.getFeatures())
        {
            return ::testing::AssertionFailure() << s.str() << "features differs";
        }

        return ::testing::AssertionSuccess();
    }
};

TEST_F(ConfigurationDataTest, fromJsonSettingsInvalidJsonStringThrows) // NOLINT
{
    try
    {
        ConfigurationData::fromJsonSettings("non json string");
    }
    catch (SulDownloaderException& e)
    {
        EXPECT_STREQ("Failed to process json message", e.what());
    }
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidButEmptyJsonStringShouldThrow) // NOLINT
{
    try
    {
        ConfigurationData::fromJsonSettings("{}");
    }
    catch (SulDownloaderException& e)
    {
        EXPECT_STREQ("Sophos Location list cannot be empty", e.what());
    }
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidAndCompleteJsonStringShouldReturnValidDataObject) // NOLINT
{
    setupFileSystemAndGetMock();
    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString("", ""));
    configurationData.verifySettingsAreValid();

    EXPECT_TRUE(configurationData.isVerified());
}

TEST_F( // NOLINT
    ConfigurationDataTest,
    fromJsonSettingsValidAndCompleteJsonStringShouldReturnValidDataObjectThatContainsExpectedData)
{
    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString("", ""));

    ConfigurationData expectedConfiguration(
        { "https://sophosupdate.sophos.com/latest/warehouse" },
        Credentials{ "administrator", "password" },
        { "https://cache.sophos.com/latest/warehouse" },
        Proxy("noproxy:"));
    expectedConfiguration.setPrimarySubscription(
        ProductSubscription{ "BaseProduct-RigidName", "9", "RECOMMENDED", "" });
    expectedConfiguration.setProductsSubscription(
        { ProductSubscription{ "PrefixOfProduct-SimulateProductA", "9", "RECOMMENDED", "" } });
    expectedConfiguration.setFeatures({ "CORE", "MDR" });
    expectedConfiguration.setSystemSslCertificatePath(m_absSystemSslPath);
    expectedConfiguration.setUpdateCacheSslCertificatePath(m_absCacheUpdatePath);
    expectedConfiguration.setCertificatePath(m_absCertificatePath);
    expectedConfiguration.setInstallArguments({ "--install-dir", "/opt/sophos-av" });
    expectedConfiguration.setLogLevel(ConfigurationData::LogLevel::NORMAL);
    EXPECT_PRED_FORMAT2(configurationDataIsEquivalent, configurationData, expectedConfiguration);
    std::string serialized = ConfigurationData::toJsonSettings(configurationData);
    ConfigurationData afterDeserialization = ConfigurationData::fromJsonSettings(serialized);
    EXPECT_PRED_FORMAT2(configurationDataIsEquivalent, configurationData, afterDeserialization);
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidStringWithNoUpdateCacheShouldReturnValidDataObject) // NOLINT
{
    setupFileSystemAndGetMock();
    ConfigurationData configurationData =
        ConfigurationData::fromJsonSettings(createJsonString(R"("https://cache.sophos.com/latest/warehouse")", ""));

    configurationData.verifySettingsAreValid();

    EXPECT_TRUE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptyUpdateCacheValueShouldFailValidation) // NOLINT
{
    setupFileSystemAndGetMock();
    ConfigurationData configurationData =
        ConfigurationData::fromJsonSettings(createJsonString(R"(https://cache.sophos.com/latest/warehouse)", ""));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidStringWithNoSophosURLsShouldThrow) // NOLINT
{
    setupFileSystemAndGetMock();
    try
    {
        ConfigurationData::fromJsonSettings(createJsonString(
            ""
            "https://sophosupdate.sophos.com/latest/warehouse"
            "",
            ""));
    }
    catch (SulDownloaderException& e)
    {
        EXPECT_STREQ("Sophos Location list cannot be empty", e.what());
    }
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptySophosUrlValueShouldFailValidation) // NOLINT
{
    setupFileSystemAndGetMock();
    ConfigurationData configurationData =
        ConfigurationData::fromJsonSettings(createJsonString("https://sophosupdate.sophos.com/latest/warehouse", ""));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidEmptyCredentialsShouldFailValidation) // NOLINT
{
    setupFileSystemAndGetMock();
    std::string oldString = R"("credential": {
                               "username": "administrator",
                               "password": "password"
                               },)";

    std::string newString = R"("credential": {
                               "username": "",
                               "password": ""
                               },)";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidMissingCredentialDetailsShouldFailValidation) // NOLINT
{
    setupFileSystemAndGetMock();
    std::string oldString = R"("credential": {
                               "username": "administrator",
                               "password": "password"
                               },)";

    std::string newString = R"("credential": {
                               },)";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidMissingCredentialsShouldFailValidation) // NOLINT
{
    setupFileSystemAndGetMock();
    std::string oldString = R"("credential": {
                               "username": "administrator",
                               "password": "password"
                               },)";

    std::string newString; // = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithMissingProxyShouldReturnValidDataObject) // NOLINT
{
    setupFileSystemAndGetMock();
    std::string oldString = R"("proxy": {
                               "url": "noproxy:",
                               "credential": {
                               "username": "",
                               "password": "",
                               "proxyType": ""
                                }
                               },)";

    std::string newString; // = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_TRUE(configurationData.isVerified());
}

TEST_F( // NOLINT
    ConfigurationDataTest,
    fromJsonSettingsValidJsonStringWithConfiguredPolicyProxyShouldReturnValidDataObject)
{
    setupFileSystemAndGetMock();
    std::string oldString = R"("proxy": {
                               "url": "noproxy:",
                               "credential": {
                               "username": "",
                               "password": "",
                               "proxyType": ""
                                }
                               },)";

    std::string newString = R"("proxy": {
                                "url": "http://dummyurl.com",
                                "credential": {
                                "username": "username",
                                "password": "password",
                                "proxyType": "2"
                                }
                                },)";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    suldownloaderdata::ProxyCredentials proxyCredentials("username", "password", "2");
    std::vector<Proxy> expectedProxyList = { Proxy("http://dummyurl.com", proxyCredentials), Proxy(Proxy::NoProxy) };
    EXPECT_EQ(configurationData.proxiesList(), expectedProxyList);
    EXPECT_TRUE(configurationData.isVerified());
}

TEST_F( // NOLINT
    ConfigurationDataTest,
    shouldRejectInvalidProxy)
{
    setupFileSystemAndGetMock();
    std::string oldString = R"("proxy": {
                               "url": "noproxy:",
                               "credential": {
                               "username": "",
                               "password": "",
                               "proxyType": ""
                                }
                               },)";

    // no username but password that does not translate to empty
    std::string newString = R"("proxy": {
                                "url": "http://dummyurl.com",
                                "credential": {
                                "username": "",
                                "password": "password",
                                "proxyType": "2"
                                }
                                },)";

    EXPECT_THROW(ConfigurationData::fromJsonSettings(createJsonString(oldString, newString)), SulDownloaderException);
    // no username and password translate to empty
    newString = R"("proxy": {
                                "url": "http://dummyurl.com",
                                "credential": {
                                "username": "",
                                "password": "CCDN+JdsRVNd+yKFqQhrmdJ856KCCLHLQxEtgwG/tD5myvTrUk/kuALeUDhL4plxGvM=",
                                "proxyType": "2"
                                }
                                },)";
    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    EXPECT_TRUE(configurationData.verifySettingsAreValid());
}

TEST_F( // NOLINT
    ConfigurationDataTest,
    fromJsonSettingsValidJsonStringWithConfiguredPolicyProxyAndEnvironmentProxyShouldReturnValidObject)
{
    setupFileSystemAndGetMock();

    setenv("HTTPS_PROXY", "https://proxy.eng.sophos:8080", 1);
    std::string oldString = R"("proxy": {
                               "url": "noproxy:",
                               "credential": {
                               "username": "",
                               "password": "",
                               "proxyType": ""
                                }
                               },)";

    std::string newString = R"("proxy": {
                                "url": "http://dummyurl.com",
                                "credential": {
                                "username": "username",
                                "password": "password",
                                "proxyType": "2"
                                }
                                },)";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    suldownloaderdata::ProxyCredentials proxyCredentials("username", "password", "2");
    std::vector<Proxy> expectedProxyList = { Proxy("http://dummyurl.com", proxyCredentials),
                                             Proxy("environment:"),
                                             Proxy(Proxy::NoProxy) };
    EXPECT_EQ(configurationData.proxiesList(), expectedProxyList);
    EXPECT_TRUE(configurationData.isVerified());
    unsetenv("HTTPS_PROXY");
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithOnlySavedProxyShouldReturnValidObject) // NOLINT
{
    auto& fileSystem = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystem, isFile(_)).WillOnce(Return(true));
    std::string savedURL("https://user:password@savedProxy.com");
    EXPECT_CALL(fileSystem, readFile(_)).WillOnce(Return(savedURL));

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString("", ""));

    configurationData.verifySettingsAreValid();

    suldownloaderdata::ProxyCredentials proxyCredentials("user", "password", "");
    std::vector<Proxy> expectedProxyList = { Proxy("savedProxy.com", proxyCredentials ), Proxy(Proxy::NoProxy) };
    EXPECT_EQ(configurationData.proxiesList(), expectedProxyList);
    EXPECT_TRUE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsUnauthenticatedProxyInSavedProxyShouldReturnValidObject) // NOLINT
{
    auto& fileSystem = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystem, isFile(_)).WillOnce(Return(true));
    std::string savedURL("http://savedProxy.com");
    EXPECT_CALL(fileSystem, readFile(_)).WillOnce(Return(savedURL));

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString("", ""));

    configurationData.verifySettingsAreValid();

    std::vector<Proxy> expectedProxyList = { Proxy("http://savedProxy.com"), Proxy(Proxy::NoProxy) };
    std::vector<Proxy> actualProxyList = configurationData.proxiesList();
    EXPECT_EQ(actualProxyList, expectedProxyList);
    EXPECT_TRUE(configurationData.isVerified());
}


TEST_F(ConfigurationDataTest, fromJsonSettingsInvalidProxyInSavedProxyShouldBeLoggedAndNotReturnValidObject) // NOLINT
{
    Common::Logging::ConsoleLoggingSetup consoleLogger;
    testing::internal::CaptureStderr();

    auto& fileSystem = setupFileSystemAndGetMock();
    EXPECT_CALL(fileSystem, isFile(_)).WillOnce(Return(true));
    std::string savedURL("www.user:password@invalidsavedProxy.com");
    EXPECT_CALL(fileSystem, readFile(_)).WillOnce(Return(savedURL));

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString("", ""));

    configurationData.verifySettingsAreValid();

    std::vector<Proxy> expectedProxyList = {Proxy(Proxy::NoProxy)};
    std::vector<Proxy> actualProxyList = configurationData.proxiesList();

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Proxy URL not in expected format."));

    EXPECT_EQ(actualProxyList, expectedProxyList);
    EXPECT_TRUE(configurationData.isVerified());
}


TEST_F(ConfigurationDataTest, proxyFromSavedProxyUrlShouldBeLoggedAndReturnNullOpt) // NOLINT
{
    Common::Logging::ConsoleLoggingSetup consoleLogger;
    testing::internal::CaptureStderr();

    std::string savedProxyURL("@http://invalidsavedProxy.com");

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString("", ""));

    std::optional<Proxy> expectedProxy = std::nullopt;
    std::optional<Proxy> actualProxy = configurationData.proxyFromSavedProxyUrl(savedProxyURL);

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Proxy URL not in expected format."));
    EXPECT_EQ(actualProxy, expectedProxy);
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptyCertificatePathWillUseDefaultOne) // NOLINT
{
    setupFileSystemAndGetMock();
    std::string oldString = m_absCertificatePath;

    std::string newString; // = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_TRUE(configurationData.isVerified());
    EXPECT_EQ(
        configurationData.getCertificatePath(),
        Common::ApplicationConfiguration::applicationPathManager().getUpdateCertificatesPath());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithMissingCertificatePathWillUseDefaultOne) // NOLINT
{
    setupFileSystemAndGetMock();

    std::string oldString = R"("certificatePath": ")" + m_absCertificatePath + R"(",)";

    std::string newString; // = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_TRUE(configurationData.isVerified());
    EXPECT_EQ(
        configurationData.getCertificatePath(),
        Common::ApplicationConfiguration::applicationPathManager().getUpdateCertificatesPath());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptySystemSslCertPathShouldFailValidation) // NOLINT
{
    std::string oldString = m_absSystemSslPath;

    std::string newString; // = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithMissingSystemSslCertPathShouldPassValidation) // NOLINT
{
    setupFileSystemAndGetMock();
    std::string oldString = R"("systemSslPath": ")" + m_absSystemSslPath + R"(",)";

    std::string newString; // = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_TRUE(configurationData.isVerified());
    EXPECT_EQ(configurationData.getSystemSslCertificatePath(), ConfigurationData::DoNotSetSslSystemPath);
}

TEST_F( // NOLINT
    ConfigurationDataTest,
    fromJsonSettingsValidJsonStringWithEmptyUpdateCacheSslCertPathShouldFailValidationWhenUsingUpdateCaches)
{
    setupFileSystemAndGetMock();
    std::string oldString = m_absCacheUpdatePath;

    std::string newString; // = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F( // NOLINT
    ConfigurationDataTest,
    fromJsonSettingsValidJsonStringWithMissingUpdateCacheSslCertPathShouldFailValidationWhenUsingUpdateCaches)
{
    setupFileSystemAndGetMock();
    std::string oldString = R"("cacheUpdateSslPath": ")" + m_absCacheUpdatePath + R"(",)";

    std::string newString; // = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F( // NOLINT
    ConfigurationDataTest,
    fromJsonSettingsValidJsonStringWithEmptyUpdateCacheSslCertPathShouldNotFailValidationWhenNotUsingUpdateCaches)
{
    setupFileSystemAndGetMock();
    std::string oldString = m_absCacheUpdatePath;

    std::string newString; // = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F( // NOLINT
    ConfigurationDataTest,
    fromJsonSettingsValidJsonStringWithMissingUpdateCacheSslCertPathShouldNotFailValidationWhenNotUsingUpdateCaches)
{
    setupFileSystemAndGetMock();
    std::string oldString = R"("cacheUpdateSslPath": ")" + m_absCacheUpdatePath + R"(",)";

    std::string newString; // = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(
    ConfigurationDataTest,
    fromJsonSettingsValidJsonStringWithMissingPrimarySubscriptionShouldFailValidation) // NOLINT
{
    setupFileSystemAndGetMock();
    std::string oldString = R"(                               "primarySubscription": {
                                "rigidName" : "BaseProduct-RigidName",
                                "baseVersion" : "9",
                                "tag" : "RECOMMENDED",
                                "fixedVersion" : ""
                                },)";
    std::string newString; //  = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringProductsWithMissingRigidNameShouldFailValidation) // NOLINT
{
    setupFileSystemAndGetMock();
    std::string oldString = R"("rigidName" : "PrefixOfProduct-SimulateProductA")";
    std::string newString = R"("rigidName" : "")";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(
    ConfigurationDataTest,
    fromJsonSettingsValidJsonStringProductsWithMissingTagAndFixedVersionShouldFailValidation) // NOLINT
{
    setupFileSystemAndGetMock();
    std::string oldString = R"({
                                "rigidName" : "PrefixOfProduct-SimulateProductA",
                                "baseVersion" : "9",
                                "tag" : "RECOMMENDED",
                                "fixedVersion" : ""
                                },)";
    std::string newString = R"({
                                "rigidName" : "PrefixOfProduct-SimulateProductA",
                                "baseVersion" : "9",
                                "tag" : "",
                                "fixedVersion" : ""
                                },)";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithEmptyInstallArgumentsShouldFailValidation) // NOLINT
{
    setupFileSystemAndGetMock();

    std::string oldString = R"("--install-dir",
                               "/opt/sophos-av")";

    std::string newString; // = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_TRUE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithMissingInstallArgumentsShouldFailValidation) // NOLINT
{
    setupFileSystemAndGetMock();
    std::string oldString = R"("installArguments": [
                               "--install-dir",
                               "/opt/sophos-av"
                               ])";

    std::string newString; // = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_TRUE(configurationData.isVerified());
}

TEST_F( // NOLINT
    ConfigurationDataTest,
    fromJsonSettingsValidJsonStringWithEmptyValueInInstallArgumentsShouldFailValidation)
{
    setupFileSystemAndGetMock();
    std::string oldString = "--install-dir";
    std::string newString; // = "";

    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    EXPECT_FALSE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithAddedUnknownDataShouldThrow) // NOLINT
{
    setupFileSystemAndGetMock();
    std::string oldString = R"("baseVersion" : "9",)";

    std::string newString = R"("baseVersion" : "9",
            "UnknownDataItem" : "UnknownString",)";

    EXPECT_THROW( // NOLINT
        ConfigurationData::fromJsonSettings(createJsonString(oldString, newString)),
        SulDownloaderException);
}

TEST_F(ConfigurationDataTest, serializeDeserialize) // NOLINT
{
    std::string originalString = createJsonString("", "");
    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(originalString);
    ConfigurationData afterSerializer =
        ConfigurationData::fromJsonSettings(ConfigurationData::toJsonSettings(configurationData));

    EXPECT_PRED_FORMAT2(configurationDataIsEquivalent, configurationData, afterSerializer);
}

TEST_F( // NOLINT
    ConfigurationDataTest,
    settingsAreValidForV2)
{
    setupFileSystemAndGetMock();
    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(createJsonString("", ""));
    EXPECT_TRUE(configurationData.verifySettingsAreValid());

    std::vector<ConfigurationData> all_invalid_cases;

    ConfigurationData noPrimarySubscriptionConfig(configurationData);
    noPrimarySubscriptionConfig.setPrimarySubscription({});
    all_invalid_cases.emplace_back(noPrimarySubscriptionConfig);

    ConfigurationData primaryWithRigidNameOnly(configurationData);
    primaryWithRigidNameOnly.setPrimarySubscription({ "rigidname", "", "", "" });
    all_invalid_cases.emplace_back(primaryWithRigidNameOnly);

    ConfigurationData primaryWithOutRigidName(configurationData);
    primaryWithOutRigidName.setPrimarySubscription({ "", "baseversion", "RECOMMENDED", "None" });
    all_invalid_cases.emplace_back(primaryWithOutRigidName);

    ConfigurationData primaryWithRigidNameAndBaseVersion(configurationData);
    primaryWithRigidNameAndBaseVersion.setPrimarySubscription({ "rigidname", "baseversion", "", "" });
    all_invalid_cases.emplace_back(primaryWithRigidNameAndBaseVersion);

    ConfigurationData productsWithRigidNameOnly(configurationData);
    productsWithRigidNameOnly.setProductsSubscription(
        { ProductSubscription("rigidname", "", "RECOMMENDED", ""), ProductSubscription("rigidname", "", "", "") });
    all_invalid_cases.emplace_back(productsWithRigidNameOnly);

    ConfigurationData noCoreFeature(configurationData);
    noCoreFeature.setFeatures({ "SAV", "MDR", "SENSOR" });
    all_invalid_cases.emplace_back(noCoreFeature);

    ConfigurationData noFeatureSet(configurationData);
    noFeatureSet.setFeatures({});
    all_invalid_cases.emplace_back(noFeatureSet);

    for (auto& configData : all_invalid_cases)
    {
        EXPECT_FALSE(configData.verifySettingsAreValid());
    }

    std::vector<ConfigurationData> all_valid_cases;
    ConfigurationData primaryWithTag(configurationData);
    primaryWithTag.setPrimarySubscription({ "rigidname", "baseversion", "RECOMMENDED", "" });
    all_valid_cases.emplace_back(primaryWithTag);

    ConfigurationData primaryWithoutBaseVersion(configurationData);
    primaryWithoutBaseVersion.setPrimarySubscription({ "rigidname", "", "RECOMMENDED", "" });
    all_valid_cases.emplace_back(primaryWithoutBaseVersion);

    ConfigurationData primaryWithTagAndFixedVersion(configurationData);
    primaryWithTagAndFixedVersion.setPrimarySubscription({ "rigidname", "", "RECOMMENDED", "9.1" });
    all_valid_cases.emplace_back(primaryWithTagAndFixedVersion);

    ConfigurationData primaryWithOnlyFixedVersion(configurationData);
    primaryWithOnlyFixedVersion.setPrimarySubscription({ "rigidname", "", "", "9.1" });
    all_valid_cases.emplace_back(primaryWithOnlyFixedVersion);

    ConfigurationData featuresContainCORE(configurationData);
    featuresContainCORE.setFeatures({ { "CORE" }, { "MDR" } });
    all_valid_cases.emplace_back(featuresContainCORE);

    ConfigurationData onlyCOREinFeatures(configurationData);
    onlyCOREinFeatures.setFeatures({ { "CORE" } });
    all_valid_cases.emplace_back(onlyCOREinFeatures);

    ConfigurationData moreThanOneProduct(configurationData);
    moreThanOneProduct.setProductsSubscription({ { "p1", "", "RECOMMENDED", "" }, { "p2", "", "", "9.1" } });
    all_valid_cases.emplace_back(moreThanOneProduct);

    ConfigurationData onlyPrimaryAvailable(configurationData);
    onlyPrimaryAvailable.setProductsSubscription({});
    all_valid_cases.emplace_back(onlyPrimaryAvailable);

    for (auto& configData : all_valid_cases)
    {
        EXPECT_TRUE(configData.verifySettingsAreValid());
    }
}

TEST_F(ConfigurationDataTest, configurationDataRetrievalShouldIgnoreUnknownFields)
{
createJsonString("", "");
    std::string serializedConfigurationDataWithUnknownField{ R"sophos({
                               "sophosURLs": [
                               "https://sophosupdate.sophos.com/latest/warehouse"
                               ],
                               "updateCache": [
                               "https://cache.sophos.com/latest/warehouse"
                               ],
                               "credential": {
                               "username": "administrator",
                               "password": "password"
                               },
                               "proxy": {
                               "url": "noproxy:",
                               "credential": {
                               "username": "",
                               "password": "",
                               "proxyType": ""
                                }
                               },
                               "manifestNames" : ["manifest.dat"],
                               "optionalManifestNames" : ["telem-manifest.dat"],
                               "installationRootPath": "absInstallationPath",
                               "certificatePath": "absCertificatePath",
                               "systemSslPath": "absSystemSslPath",
                               "cacheUpdateSslPath": "absCacheUpdatePath",
                               "primarySubscription": {
                                "rigidName" : "BaseProduct-RigidName",
                                "baseVersion" : "9",
                                "tag" : "RECOMMENDED",
                                "fixedVersion" : ""
                                },
                                "products": [
                                {
                                "rigidName" : "PrefixOfProduct-SimulateProductA",
                                "baseVersion" : "9",
                                "tag" : "RECOMMENDED",
                                "fixedVersion" : ""
                                },
                                ],
                                "features": ["CORE", "MDR"],
                               "installArguments": [
                               "--install-dir",
                               "/opt/sophos-spl"
                               ],
                               "fieldUnknown": "anyvalue"
                               })sophos" };
    ConfigurationData configurationData = ConfigurationData::fromJsonSettings(serializedConfigurationDataWithUnknownField); 
    std::vector<std::string> features = configurationData.getFeatures(); 
    std::vector<std::string> expected_features{{std::string{"CORE"}, std::string{"MDR"}}}; 
    EXPECT_EQ(features, expected_features);  

}
