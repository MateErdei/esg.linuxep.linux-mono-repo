// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/Policy/PolicyParseException.h"
#include "SulDownloader/suldownloaderdata/ConfigurationData.h"
#include "tests/Common/Helpers/ConfigurationDataBase.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/MockFileSystem.h"

using namespace Common::Policy;
using namespace SulDownloader;
using namespace SulDownloader::suldownloaderdata;

class ConfigurationDataTest : public ConfigurationDataBase
{
public:
    void TearDown() override
    {
        Tests::restoreFileSystem();
    }

    MockFileSystem& setupFileSystemAndGetMock()
    {
        using ::testing::Ne;
        Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL, "/installroot");

        auto filesystemMock = std::make_unique<NiceMock<MockFileSystem>>();
        ON_CALL(*filesystemMock, isDirectory(Common::ApplicationConfiguration::applicationPathManager().sophosInstall())).WillByDefault(Return(true));
        ON_CALL(*filesystemMock, isDirectory(Common::ApplicationConfiguration::applicationPathManager().getLocalWarehouseStoreDir())).WillByDefault(Return(true));

        std::string empty;
        ON_CALL(*filesystemMock, exists(empty)).WillByDefault(Return(false));
        ON_CALL(*filesystemMock, exists(Ne(empty))).WillByDefault(Return(true));

        auto* borrowedPtr = filesystemMock.get();
        m_replacer.replace(std::move(filesystemMock));
        return *borrowedPtr;
    }

    Tests::ScopedReplaceFileSystem m_replacer;

    static ::testing::AssertionResult configurationDataIsEquivalent(
        const char* m_expr,
        const char* n_expr,
        const Common::Policy::UpdateSettings& expected,
        const Common::Policy::UpdateSettings& resulted)
    {
        std::stringstream s;
        s << m_expr << " and " << n_expr << " failed: ";

        if (expected.getLogLevel() != resulted.getLogLevel())
        {
            return ::testing::AssertionFailure() << s.str() << "log level differs";
        }

        if (expected.getSophosSusURL() != resulted.getSophosSusURL())
        {
            return ::testing::AssertionFailure() << s.str() << "sus url differ";
        }

        if (expected.getSophosCDNURLs() != resulted.getSophosCDNURLs())
        {
            return ::testing::AssertionFailure() << s.str() << "cdn urls differ";
        }

        if (expected.getLocalUpdateCacheHosts() != resulted.getLocalUpdateCacheHosts())
        {
            return ::testing::AssertionFailure() << s.str() << "update cache urls differ";
        }

        if (expected.getPolicyProxy().getUrl() != resulted.getPolicyProxy().getUrl())
        {
            return ::testing::AssertionFailure()
                   << s.str() << "proxy urls differ " << expected.getPolicyProxy().getUrl()
                   << " != " << resulted.getPolicyProxy().getUrl();
        }

        if (expected.getPolicyProxy().getCredentials() != resulted.getPolicyProxy().getCredentials())
        {
            return ::testing::AssertionFailure() << s.str() << "proxy credentials differ ";
        }

        if (expected.getPolicyProxy() != resulted.getPolicyProxy())
        {
            return ::testing::AssertionFailure() << s.str() << "policy proxy differs";
        }

        if (expected.getLocalWarehouseRepository() != resulted.getLocalWarehouseRepository())
        {
            return ::testing::AssertionFailure() << s.str() << "local warehouse repository differs";
        }

        if (expected.getLocalDistributionRepository() != resulted.getLocalDistributionRepository())
        {
            return ::testing::AssertionFailure() << s.str() << "local distribution repository differs";
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

        if (expected.getDoForcedUpdate() != resulted.getDoForcedUpdate())
        {
            return ::testing::AssertionFailure() << s.str() << "forceUpdate differs";
        }

        if (expected.getDoPausedForcedUpdate() != resulted.getDoPausedForcedUpdate())
        {
            return ::testing::AssertionFailure() << s.str() << "forcePausedUpdate differs";
        }

        if (expected.getUseSdds3DeltaV3() != resulted.getUseSdds3DeltaV3())
        {
            return ::testing::AssertionFailure() << s.str() << "useSdds3DeltaV3 differs";
        }
        return ::testing::AssertionSuccess();
    }
};



TEST_F(
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

    auto configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    auto proxy = configurationData.getPolicyProxy();
    EXPECT_FALSE(proxy.empty());
    EXPECT_EQ(proxy.getUrl(), "http://dummyurl.com");

    ProxyCredentials proxyCredentials("username", "password", "2");
    std::vector<Proxy> expectedProxyList = { Proxy("http://dummyurl.com", proxyCredentials), Proxy(NoProxy) };
    auto proxyList = ConfigurationData::proxiesList(configurationData);
    EXPECT_EQ(proxyList.size(), expectedProxyList.size());
    EXPECT_EQ(proxyList, expectedProxyList);
    EXPECT_TRUE(configurationData.isVerified());
}

TEST_F(
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

    EXPECT_THROW(ConfigurationData::fromJsonSettings(createJsonString(oldString, newString)), PolicyParseException);
    // no username and password translate to empty
    newString = R"("proxy": {
                                "url": "http://dummyurl.com",
                                "credential": {
                                "username": "",
                                "password": "CCDN+JdsRVNd+yKFqQhrmdJ856KCCLHLQxEtgwG/tD5myvTrUk/kuALeUDhL4plxGvM=",
                                "proxyType": "2"
                                }
                                },)";
    auto configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    EXPECT_TRUE(configurationData.verifySettingsAreValid());
}

TEST_F(
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

    auto configurationData = ConfigurationData::fromJsonSettings(createJsonString(oldString, newString));

    configurationData.verifySettingsAreValid();

    ProxyCredentials proxyCredentials("username", "password", "2");
    std::vector<Proxy> expectedProxyList = { Proxy("http://dummyurl.com", proxyCredentials),
                                             Proxy("environment:"),
                                             Proxy(NoProxy) };
    auto proxyList = ConfigurationData::proxiesList(configurationData);
    EXPECT_EQ(proxyList.size(), expectedProxyList.size());
    EXPECT_EQ(proxyList, expectedProxyList);
    EXPECT_TRUE(configurationData.isVerified());
    unsetenv("HTTPS_PROXY");
}

TEST_F(ConfigurationDataTest, fromJsonSettingsValidJsonStringWithOnlySavedProxyShouldReturnValidObject)
{
    auto& fileSystem = setupFileSystemAndGetMock();

    std::string savedProxyFilePath =
        Common::ApplicationConfiguration::applicationPathManager().getSavedEnvironmentProxyFilePath();
    std::string currentProxyFilePath =
        Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFilePath();
    EXPECT_CALL(fileSystem, isFile(savedProxyFilePath)).WillOnce(Return(true));
    EXPECT_CALL(fileSystem, isFile(currentProxyFilePath)).WillOnce(Return(false));
    std::vector<std::string> savedURL{"https://user:password@savedProxy.com"};
    EXPECT_CALL(fileSystem, readLines(savedProxyFilePath)).WillOnce(Return(savedURL));

    auto configurationData = ConfigurationData::fromJsonSettings(createJsonString("", ""));

    configurationData.verifySettingsAreValid();

    ProxyCredentials proxyCredentials("user", "password", "");
    std::vector<Proxy> expectedProxyList = { Proxy("https://savedProxy.com", proxyCredentials), Proxy(NoProxy) };
    auto proxyList = ConfigurationData::proxiesList(configurationData);
    EXPECT_EQ(proxyList.size(), expectedProxyList.size());
    EXPECT_EQ(proxyList, expectedProxyList);
    EXPECT_TRUE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsUnauthenticatedProxyInSavedProxyShouldReturnValidObject)
{
    auto& fileSystem = setupFileSystemAndGetMock();
    std::string savedProxyFilePath =
        Common::ApplicationConfiguration::applicationPathManager().getSavedEnvironmentProxyFilePath();
    std::string currentProxyFilePath =
        Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFilePath();
    EXPECT_CALL(fileSystem, isFile(savedProxyFilePath)).WillOnce(Return(true));
    EXPECT_CALL(fileSystem, isFile(currentProxyFilePath)).WillOnce(Return(false));

    std::vector<std::string> savedURL{"http://savedProxy.com"};
    EXPECT_CALL(fileSystem, readLines(savedProxyFilePath)).WillOnce(Return(savedURL));

    auto configurationData = ConfigurationData::fromJsonSettings(createJsonString("", ""));

    configurationData.verifySettingsAreValid();

    std::vector<Proxy> expectedProxyList = { Proxy("http://savedProxy.com"), Proxy(NoProxy) };
    auto actualProxyList = ConfigurationData::proxiesList(configurationData);
    EXPECT_EQ(actualProxyList.size(), expectedProxyList.size());
    EXPECT_EQ(actualProxyList, expectedProxyList);
    EXPECT_TRUE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, fromJsonSettingsInvalidProxyInSavedProxyShouldBeLoggedAndNotReturnValidObject)
{
    Common::Logging::ConsoleLoggingSetup consoleLogger;
    testing::internal::CaptureStderr();
    auto& fileSystem = setupFileSystemAndGetMock();
    std::string savedProxyFilePath =
        Common::ApplicationConfiguration::applicationPathManager().getSavedEnvironmentProxyFilePath();
    std::string currentProxyFilePath =
        Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFilePath();
    EXPECT_CALL(fileSystem, isFile(savedProxyFilePath)).WillOnce(Return(true));
    EXPECT_CALL(fileSystem, isFile(currentProxyFilePath)).WillOnce(Return(false));
    std::vector<std::string> savedURL{"www.user:password@invalidsavedProxy.com"};
    EXPECT_CALL(fileSystem, readLines(savedProxyFilePath)).WillOnce(Return(savedURL));

    auto configurationData = ConfigurationData::fromJsonSettings(createJsonString("", ""));

    configurationData.verifySettingsAreValid();

    std::vector<Proxy> expectedProxyList = { Proxy(NoProxy) };
    auto actualProxyList = ConfigurationData::proxiesList(configurationData);

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Proxy URL not in expected format."));

    EXPECT_EQ(actualProxyList, expectedProxyList);
    EXPECT_TRUE(configurationData.isVerified());
}

TEST_F(ConfigurationDataTest, proxyFromSavedProxyUrlShouldBeLoggedAndReturnNullOpt)
{
    Common::Logging::ConsoleLoggingSetup consoleLogger;
    testing::internal::CaptureStderr();

    std::string savedProxyURL("@http://invalidsavedProxy.com");

    std::optional<Proxy> expectedProxy = std::nullopt;
    std::optional<Proxy> actualProxy = ConfigurationData::proxyFromSavedProxyUrl(savedProxyURL);

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Proxy URL not in expected format."));
    EXPECT_EQ(actualProxy, expectedProxy);
}



TEST_F(ConfigurationDataTest, serializeDeserialize)
{
    std::string originalString = createJsonString("", "");
    auto configurationData = ConfigurationData::fromJsonSettings(originalString);
    auto afterSerializer =
        ConfigurationData::fromJsonSettings(ConfigurationData::toJsonSettings(configurationData));

    EXPECT_PRED_FORMAT2(configurationDataIsEquivalent, configurationData, afterSerializer);
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
    auto configurationData = ConfigurationData::fromJsonSettings(serializedConfigurationDataWithUnknownField);
    std::vector<std::string> features = configurationData.getFeatures(); 
    std::vector<std::string> expected_features{{std::string{"CORE"}, std::string{"MDR"}}}; 
    EXPECT_EQ(features, expected_features);
}


// currentMcsProxy:

TEST_F(ConfigurationDataTest, currentMcsProxyReturnsNulloptIfCurrentProxyFileMissing)
{
    auto& fileSystem = setupFileSystemAndGetMock();

    std::string currentProxyFilePath =
        Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFilePath();
    EXPECT_CALL(fileSystem, isFile(currentProxyFilePath)).WillOnce(Return(false));

    std::optional<Proxy> actualProxy = ConfigurationData::currentMcsProxy();
    EXPECT_EQ(actualProxy, std::nullopt);
}

TEST_F(ConfigurationDataTest, currentMcsProxyReturnsNulloptIfCurrentProxyFileEmpty)
{
    auto& fileSystem = setupFileSystemAndGetMock();

    std::string currentProxyFilePath =
        Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFilePath();
    EXPECT_CALL(fileSystem, isFile(currentProxyFilePath)).WillOnce(Return(true));
    EXPECT_CALL(fileSystem, readFile(currentProxyFilePath)).WillOnce(Return(""));

    std::optional<Proxy> expectedProxy = std::nullopt;
    std::optional<Proxy> actualProxy = ConfigurationData::currentMcsProxy();
    EXPECT_EQ(actualProxy, expectedProxy);
}

TEST_F(ConfigurationDataTest, proxyIsExtractedFromCurrentProxyFileAndLogsAddress)
{
    Common::Logging::ConsoleLoggingSetup consoleLogger;

    auto& fileSystem = setupFileSystemAndGetMock();

    std::string currentProxyFilePath =
        Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFilePath();

    EXPECT_CALL(fileSystem, isFile(currentProxyFilePath)).WillOnce(Return(true));
    std::string currentProxy = R"({"proxy": "10.55.36.235:3129", "credentials": "CCBv6oin2yWCd1PUWKpab1GcYXBB0iC1bwnajy0O1XVvOrRTTFGiruMEz5auCd8BpbE="})";
    EXPECT_CALL(fileSystem, readFile(currentProxyFilePath)).WillOnce(Return(currentProxy));

    std::optional<Proxy> expectedProxy = Proxy("10.55.36.235:3129", ProxyCredentials("user","password",""));

    testing::internal::CaptureStderr();
    std::optional<Proxy> actualProxy = ConfigurationData::currentMcsProxy();
    std::string logMessage = testing::internal::GetCapturedStderr();

    ASSERT_THAT(logMessage, ::testing::HasSubstr("Proxy address from current_proxy file: 10.55.36.235:3129"));
    ASSERT_THAT(logMessage, ::testing::HasSubstr("Proxy in current_proxy has credentials"));
    ASSERT_THAT(logMessage, ::testing::HasSubstr("Deobfuscated credentials from current_proxy"));
    ASSERT_EQ(actualProxy, expectedProxy);
}
