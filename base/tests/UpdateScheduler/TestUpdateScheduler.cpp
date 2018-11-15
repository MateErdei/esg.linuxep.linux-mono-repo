/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <UpdateSchedulerImpl/UpdateSchedulerProcessor.h>
#include "MockAsyncDownloaderRunner.h"
#include "MockCronSchedulerThread.h"
#include <tests/Common/PluginApiImpl/MockApiBaseServices.h>
#include <UpdateSchedulerImpl/Logger.h>
#include <gmock/gmock-matchers.h>
#include <modules/UpdateSchedulerImpl/LoggingSetup.h>
#include <tests/Common/FileSystemImpl/MockFileSystem.h>
#include <Common/FileSystemImpl/FileSystemImpl.h>
#include <modules/Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <modules/Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <future>
#include <modules/Common/ProcessImpl/ProcessImpl.h>
#include <tests/Common/ProcessImpl/MockProcess.h>
#include <Common/UtilityImpl/StringUtils.h>

namespace
{
    std::string updatePolicyWithProxy{R"sophos(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
  <csc:Comp RevID="f6babe12a13a5b2134c5861d01aed0eaddc20ea374e3a717ee1ea1451f5e2cf6" policyType="1"/>
  <AUConfig platform="Linux">
    <sophos_address address="http://es-web.sophos.com/update"/>
    <primary_location>
      <server BandwidthLimit="256" AutoDial="false" Algorithm="Clear" UserPassword="54m5ung" UserName="QA940267" UseSophos="true" UseHttps="false" UseDelta="true" ConnectionAddress="" AllowLocalConfig="false"/>
      <proxy ProxyType="2" ProxyUserPassword="CCC4Fcz2iNaH44sdmqyLughrajL7svMPTbUZc/Q4c7yAtSrdM03lfO33xI0XKNU4IBY=" ProxyUserName="TestUser" ProxyPortNumber="8080" ProxyAddress="uk-abn-wpan-1.green.sophos" AllowLocalConfig="false"/>
    </primary_location>
    <secondary_location>
      <server BandwidthLimit="256" AutoDial="false" Algorithm="" UserPassword="" UserName="" UseSophos="false" UseHttps="false" UseDelta="true" ConnectionAddress="" AllowLocalConfig="false"/>
      <proxy ProxyType="0" ProxyUserPassword="" ProxyUserName="" ProxyPortNumber="0" ProxyAddress="" AllowLocalConfig="false"/>
    </secondary_location>
    <schedule AllowLocalConfig="false" SchedEnable="true" Frequency="50" DetectDialUp="false"/>
    <logging AllowLocalConfig="false" LogLevel="50" LogEnable="true" MaxLogFileSize="1"/>
    <bootstrap Location="" UsePrimaryServerAddress="true"/>
    <cloud_subscription RigidName="5CF594B0-9FED-4212-BA91-A4077CB1D1F3" Tag="RECOMMENDED" BaseVersion="10"/>
    <cloud_subscriptions>
      <subscription Id="Base" RigidName="5CF594B0-9FED-4212-BA91-A4077CB1D1F3" Tag="RECOMMENDED" BaseVersion="10"/>
      <subscription Id="Base" RigidName="5CF594B0-9FED-4212-BA91-A4077CB1D1F3" Tag="RECOMMENDED" BaseVersion="9"/>
    </cloud_subscriptions>
    <delay_supplements enabled="true"/>
  </AUConfig>
  <Features>
    <Feature id="APPCNTRL"/>
    <Feature id="AV"/>
    <Feature id="CORE"/>
    <Feature id="DLP"/>
    <Feature id="DVCCNTRL"/>
    <Feature id="EFW"/>
    <Feature id="HBT"/>
    <Feature id="MTD"/>
    <Feature id="NTP"/>
    <Feature id="SAV"/>
    <Feature id="SDU"/>
    <Feature id="WEBCNTRL"/>
  </Features>
  <intelligent_updating Enabled="false" SubscriptionPolicy="2DD71664-8D18-42C5-B3A0-FF0D289265BF"/>
  <customer id="9972e4cf-dba3-e4ab-19dc-77619acac988"/>
</AUConfigurations>
)sophos"};
    std::string updatePolicyWithCache{R"sophos(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
  <csc:Comp RevID="b6a8fe2c0ce016c949016a5da2b7a089699271290ef7205d5bea0986768485d9" policyType="1"/>
  <AUConfig platform="Linux">
    <sophos_address address="http://es-web.sophos.com/update"/>
    <primary_location>
      <server BandwidthLimit="0" AutoDial="false" Algorithm="Clear" UserPassword="xxxxxx" UserName="W2YJXI6FED" UseSophos="true" UseHttps="true" UseDelta="true" ConnectionAddress="http://dci.sophosupd.com/cloudupdate" AllowLocalConfig="false"/>
      <proxy ProxyType="0" ProxyUserPassword="" ProxyUserName="" ProxyPortNumber="0" ProxyAddress="" AllowLocalConfig="false"/>
    </primary_location>
    <secondary_location>
      <server BandwidthLimit="0" AutoDial="false" Algorithm="" UserPassword="" UserName="" UseSophos="false" UseHttps="true" UseDelta="true" ConnectionAddress="" AllowLocalConfig="false"/>
      <proxy ProxyType="0" ProxyUserPassword="" ProxyUserName="" ProxyPortNumber="0" ProxyAddress="" AllowLocalConfig="false"/>
    </secondary_location>
    <schedule AllowLocalConfig="false" SchedEnable="true" Frequency="50" DetectDialUp="false"/>

    <logging AllowLocalConfig="false" LogLevel="50" LogEnable="true" MaxLogFileSize="1"/>
    <bootstrap Location="" UsePrimaryServerAddress="true"/>
    <cloud_subscription RigidName="5CF594B0-9FED-4212-BA91-A4077CB1D1F3" Tag="RECOMMENDED" BaseVersion="10"/>
    <cloud_subscriptions>
      <subscription Id="Base" RigidName="5CF594B0-9FED-4212-BA91-A4077CB1D1F3" Tag="RECOMMENDED" BaseVersion="10"/>
      <subscription Id="Base" RigidName="5CF594B0-9FED-4212-BA91-A4077CB1D1F3" Tag="RECOMMENDED" BaseVersion="9"/>
    </cloud_subscriptions>
    <delay_supplements enabled="false"/>
  </AUConfig>
  <Features>
    <Feature id="APPCNTRL"/>
    <Feature id="AV"/>
    <Feature id="CORE"/>
    <Feature id="DLP"/>
    <Feature id="DVCCNTRL"/>
    <Feature id="EFW"/>
    <Feature id="HBT"/>
    <Feature id="MTD"/>
    <Feature id="NTP"/>
    <Feature id="SAV"/>
    <Feature id="SDU"/>
    <Feature id="WEBCNTRL"/>
  </Features>
  <intelligent_updating Enabled="false" SubscriptionPolicy="2DD71664-8D18-42C5-B3A0-FF0D289265BF"/>
  <update_cache>
    <intermediate_certificates>
      <intermediate_certificate id="cfe086255488e4a45ebe10956bb3606c7ab5dc6a.crt">-----BEGIN CERTIFICATE-----
MIIDxDCCAqygAwIBAgIQEFgFEJ0SYXZx+dHI2UTIVzANBgkqhkiG9w0BAQsFADBh&#13;
MQswCQYDVQQGEwJHQjEPMA0GA1UEBwwGT3hmb3JkMQ8wDQYDVQQKDAZTb3Bob3Mx&#13;
EjAQBgNVBAsMCUhlYXJ0YmVhdDEcMBoGA1UEAwwTSGVhcnRiZWF0LWV1LXdlc3Qt&#13;
MTAeFw0xNzExMDkwNzE1MDRaFw0xOTExMTMwNzE1MDRaMIGyMQswCQYDVQQGEwJH&#13;
QjEPMA0GA1UEBwwGT3hmb3JkMQ8wDQYDVQQKDAZTb3Bob3MxEjAQBgNVBAsMCUhl&#13;
YXJ0YmVhdDEYMBYGA1UEAwwPUHJvZHVjdCBMaWNlbnNlMTgwNgYDVQQDDC9DdXN0&#13;
b21lcklEOjRiNGNhM2JhLWMxNDQtNDQ0Ny04MDUwLTZjOTZhNzEwNGMxMTEZMBcG&#13;
A1UEAwwQU29waG9zIEhlYXJ0YmVhdDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCC&#13;
AQoCggEBAJQSXwmoOsdPRBQUI8f8i7Z6+UhNwvKPoM/kyMbwf2DG8FYPrpiG6uhR&#13;
JT7SBRGcWZstWIsafrj+RrXrXjapm0xGqSNc1nIuxQDcXJmmq74XCGedHiQGLF+f&#13;
BEDvdYd2KHrYwrg0nGuJB8tPz2s2MRcQLnx30eBL5KAMOkMoZMbkNnCHI8FycTDS&#13;
jwjqjN9u3JOMvbA1YEVDonDe39v0Oxj1sY93Oh3xG44496vhhy8r6nwdY9L9DCwn&#13;
OU/1wzGapNgDl7ox1JIFIlq+z5iaeGLVp2peVDtPsK8pJBISr6NeG0Mm6wMKkMre&#13;
w/Nm8Nu9bV+ocbG/xYncbjP8BYwhkYMCAwEAAaMmMCQwEgYDVR0TAQH/BAgwBgEB&#13;
/wIBADAOBgNVHQ8BAf8EBAMCAQYwDQYJKoZIhvcNAQELBQADggEBAFXtbg/c7t1t&#13;
MAZFFKUgtkEB5Hh2OBKPS5F87BPfZQD1RLUcbnB+mIuqpAIqHE/3ovtV7KBe0wFz&#13;
WgcN08w0Q4yAJV5eNsBZiqZYYRweB157vo/6oY1wvqB6D0tTcfSDG8/kthLITecC&#13;
C9jtK5rIwF4pAA4WKkAY6VT+uDCZU+Hejsutc7Ey9Tpon2fCpcmd6HizzCNWeRnG&#13;
6YtOsPIjQMToz7b3SnpE2m1zk4jmchQX55yn0rEAf2f/XqblPaEPpoySSNhx/uTg&#13;
zhGZIvpcyR9tIr43hyx2iR+E1TH7DqGeeHcbr8pQyd6UAGK9TAA1oDDuejXpQ/GY&#13;
c2uueUDEuWQ=&#13;

-----END CERTIFICATE-----</intermediate_certificate>
      <intermediate_certificate id="230d7f14e899d07cd0581d150cbda05826b59bef.crt">-----BEGIN CERTIFICATE-----
MIIDnTCCAoWgAwIBAgIBAjANBgkqhkiG9w0BAQsFADB/MQswCQYDVQQGEwJHQjEP&#13;
MA0GA1UEBwwGT3hmb3JkMQ8wDQYDVQQKDAZTb3Bob3MxFDASBgNVBAsMC0VuZ2lu&#13;
ZWVyaW5nMTgwNgYDVQQDDC9Tb3Bob3MgSGVhcnRiZWF0IE5vdC1Gb3ItUmVsZWFz&#13;
ZSBSb290IEF1dGhvcml0eTAeFw0xNTA2MDQyMTQyMTNaFw0zNTA2MDQyMTQyMTNa&#13;
MGExCzAJBgNVBAYTAkdCMQ8wDQYDVQQHDAZPeGZvcmQxDzANBgNVBAoMBlNvcGhv&#13;
czESMBAGA1UECwwJSGVhcnRiZWF0MRwwGgYDVQQDDBNIZWFydGJlYXQtZXUtd2Vz&#13;
dC0xMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAlJSxtUjPwyFrldV4&#13;
5TJlhP0Kmko3NyHyRqI9xsMRzJ6ZYgJWx1gpPkoT0L4EU+ZStdKbuizL5lhZfBQj&#13;
FmxMBLss+tD3WUIE1LQY2Ge+XzbhpTv2QPEslf/nX6T7uEKf3Sb7nsq6JbV3wOjV&#13;
R4rsehbUT5tPb0QZAXBu3Cej5ppQrEeQMLeVdU9js8pAKK1HhHJPXcQEjw2SPRGa&#13;
S8UnXkK5nd+myQYAgrCtplD0Ul35U2Kc0lRsyVeQsupHlNN5we7HfSI4TrPHox7B&#13;
rjAEmQ77c9uP6PbXqY8buo8TlT33eoUYJwFd9ivXutFRf0l8QO2u6Jg+W7eCQqp9&#13;
uNYHYwIDAQABo0IwQDAdBgNVHQ4EFgQUrv/9VM4ZJ2TRU4LJ6n3u0YPY8bEwDwYD&#13;
VR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMCAQYwDQYJKoZIhvcNAQELBQADggEB&#13;
ALIoFNI+pvj57lL83AS1rZX2V6CvGjCiFpR0YfJXlKCMFX9QOm1zT8OKkaLdMAsm&#13;
BQZuwhL2k3byk5b8gHYn9IcVE55RC5F2WUsdwke27BIalSSPNitc5sIxKBoc4BLS&#13;
2pkhIDwqjTqX9HM31IVT/GlnYqubh1kigqgdOQOIRfCKpb17TZoRQHbC7N5gOoa7&#13;
xNwKx/Z1lc6IsUFSFuevVRIhlY0FL0xBIKveMYIBJFgZGY+hRCevqIu1sR0OYNHg&#13;
nJwgZ6PRCYPMsEntjoFEy5IoVheP/1yHCC/1seH9nhtGe7xzjuUJHwfPm9kBOpCU&#13;
zJqTw7msuYhp6kd4xCw7ccc=&#13;

-----END CERTIFICATE-----</intermediate_certificate>
      <intermediate_certificate id="33d6a435957397fc9336c8633445aa33e1774500.crt">-----BEGIN CERTIFICATE-----
MIIDuzCCAqOgAwIBAgIBATANBgkqhkiG9w0BAQsFADB/MQswCQYDVQQGEwJHQjEP&#13;
MA0GA1UEBwwGT3hmb3JkMQ8wDQYDVQQKDAZTb3Bob3MxFDASBgNVBAsMC0VuZ2lu&#13;
ZWVyaW5nMTgwNgYDVQQDDC9Tb3Bob3MgSGVhcnRiZWF0IE5vdC1Gb3ItUmVsZWFz&#13;
ZSBSb290IEF1dGhvcml0eTAeFw0xNTA2MDQyMTQyMDJaFw0zNTA2MDQyMTQyMDJa&#13;
MH8xCzAJBgNVBAYTAkdCMQ8wDQYDVQQHDAZPeGZvcmQxDzANBgNVBAoMBlNvcGhv&#13;
czEUMBIGA1UECwwLRW5naW5lZXJpbmcxODA2BgNVBAMML1NvcGhvcyBIZWFydGJl&#13;
YXQgTm90LUZvci1SZWxlYXNlIFJvb3QgQXV0aG9yaXR5MIIBIjANBgkqhkiG9w0B&#13;
AQEFAAOCAQ8AMIIBCgKCAQEAtQqlMqhJNYlTp/4zElmB7WOEsFUCLVD+Kzneh733&#13;
JbpL+RgRQ9qn76+J9vcFPG+eA5zf8M8Mbmb7IBEuLMmD5erT00AwoH8Vr4UP9Wb4&#13;
wPn8QzI+WO5oO3NjGvVGD/cufjBH9zqi8oiQwYELrz8fHXTxkCAJHcLp4tM86SQc&#13;
QnkWQv9TEWbGgrhJnMg3DAY8pERdLBhjVHPK15ZwyvMlqISBVOm9k8c81fwx4pfG&#13;
b4O+1CylPUKqreUXw7T6HB85+ym3TAkq9eeiJA1BkjdhWdpewIII5U955eaWJXaA&#13;
ra1jdNb21JAbrwy67Vp4v3uQ+62pOkubThlaQ+qIajJsHwIDAQABo0IwQDAdBgNV&#13;
HQ4EFgQUI6jNsBYvHqSsWqJfTrCMizzCHx0wDwYDVR0TAQH/BAUwAwEB/zAOBgNV&#13;
HQ8BAf8EBAMCAQYwDQYJKoZIhvcNAQELBQADggEBAE4EtR6dARf1ZqnfQlbjKubN&#13;
GGrDVv3GOmPenAH4AcCrXNe6ObIadJFVKWRYMvvMdb4GxxEts29OZdHnFK+Vvrqh&#13;
yUV0o4It14BBNjLIApo9eddShyDEZKGusQ33S9XYCIG6Yan59jOo2rvsntwi829v&#13;
B5yoGhSoxOaXlExk9YAGXaGmoaJ228iF2vuwmgSHJVxUy02AwiqqEwUy/MGL8877&#13;
wFkMtR8hrPVLP0hcHuzWN2cBmrl0C6TeKufqbZBqb/MPn2LWzKcvF44xs3k7uP/H&#13;
JWfkv6Tu5jsYGNkN3BSW0x/qjwz7XCSk2ZZxbCgZSq6LpB31sqZctnUxrYSpcdc=&#13;

-----END CERTIFICATE-----</intermediate_certificate>
    </intermediate_certificates>
    <locations>
      <location hostname="2k12-64-ld55-df.eng.sophos:8191" priority="1" id="4092822d-0925-4deb-9146-fbc8532f8c55"/>
      <location hostname="maineng2.eng.sophos:8191" priority="0" id="d4739ed5-b66a-4a21-9218-4fd0dfe4dcbd"/>
      <location hostname="w2k8r2-std-en-df.eng.sophos:8191" priority="1" id="0e1e598b-d6d6-4e4b-9425-9fefd277a353"/>
    </locations>
  </update_cache>
  <customer id="4b4ca3ba-c144-4447-8050-6c96a7104c11"/>
</AUConfigurations>
)sophos"};

    std::string updateAction{R"sophos(<?xml version='1.0'?><action type="sophos.mgt.action.ALCForceUpdate"/>)sophos"};
    std::string downloadReport{R"sophos({ "startTime": "20180810 10:00:00",
 "finishTime": "20180810 11:00:00",
 "syncTime": "20180810 11:00:00",
 "status": "SUCCESS",
 "sulError": "",
 "errorDescription": "",
 "urlSource": "cache1",
 "products": [
  {
   "rigidName": "BaseRigidName",
   "productName": "BaseName",
   "installedVersion": "0.5.0",
   "downloadVersion": "0.5.0",
   "errorDescription": "",
   "productStatus": "UPGRADED"
  },
  {
   "rigidName": "PluginRigidName",
   "productName": "PluginName",
   "installedVersion": "0.5.0",
   "downloadVersion": "0.5.0",
   "errorDescription": "",
   "productStatus": "UPGRADED"
  }
 ]
}
)sophos"};
}

static const std::string updatePolicyWithScheduledUpdate{R"sophos(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
  <csc:Comp RevID="f6babe12a13a5b2134c5861d01aed0eaddc20ea374e3a717ee1ea1451f5e2cf6" policyType="1"/>
  <AUConfig platform="Linux">
    <sophos_address address="http://es-web.sophos.com/update"/>
    <primary_location>
      <server BandwidthLimit="256" AutoDial="false" Algorithm="Clear" UserPassword="54m5ung" UserName="QA940267" UseSophos="true" UseHttps="false" UseDelta="true" ConnectionAddress="" AllowLocalConfig="false"/>
      <proxy ProxyType="2" ProxyUserPassword="CCC4Fcz2iNaH44sdmqyLughrajL7svMPTbUZc/Q4c7yAtSrdM03lfO33xI0XKNU4IBY=" ProxyUserName="TestUser" ProxyPortNumber="8080" ProxyAddress="uk-abn-wpan-1.green.sophos" AllowLocalConfig="false"/>
    </primary_location>
    <secondary_location>
      <server BandwidthLimit="256" AutoDial="false" Algorithm="" UserPassword="" UserName="" UseSophos="false" UseHttps="false" UseDelta="true" ConnectionAddress="" AllowLocalConfig="false"/>
      <proxy ProxyType="0" ProxyUserPassword="" ProxyUserName="" ProxyPortNumber="0" ProxyAddress="" AllowLocalConfig="false"/>
    </secondary_location>
    <schedule AllowLocalConfig="false" SchedEnable="true" Frequency="40" DetectDialUp="false"/>
    <logging AllowLocalConfig="false" LogLevel="50" LogEnable="true" MaxLogFileSize="1"/>
    <bootstrap Location="" UsePrimaryServerAddress="true"/>
    <cloud_subscription RigidName="5CF594B0-9FED-4212-BA91-A4077CB1D1F3" Tag="RECOMMENDED" BaseVersion="10"/>
    <cloud_subscriptions>
      <subscription Id="Base" RigidName="5CF594B0-9FED-4212-BA91-A4077CB1D1F3" Tag="RECOMMENDED" BaseVersion="10"/>
      <subscription Id="Base" RigidName="5CF594B0-9FED-4212-BA91-A4077CB1D1F3" Tag="RECOMMENDED" BaseVersion="9"/>
    </cloud_subscriptions>
    <delay_supplements enabled="true"/>
    <delay_updating Day="Saturday" Time="09:41:00"/>
  </AUConfig>
  <Features>
    <Feature id="APPCNTRL"/>
    <Feature id="AV"/>
    <Feature id="CORE"/>
    <Feature id="DLP"/>
    <Feature id="DVCCNTRL"/>
    <Feature id="EFW"/>
    <Feature id="HBT"/>
    <Feature id="MTD"/>
    <Feature id="NTP"/>
    <Feature id="SAV"/>
    <Feature id="SDU"/>
    <Feature id="WEBCNTRL"/>
  </Features>
  <intelligent_updating Enabled="false" SubscriptionPolicy="2DD71664-8D18-42C5-B3A0-FF0D289265BF"/>
  <customer id="9972e4cf-dba3-e4ab-19dc-77619acac988"/>
</AUConfigurations>
)sophos"};


using namespace UpdateSchedulerImpl;
using namespace UpdateScheduler;
using namespace Common::PluginApi;

class TestUpdateScheduler
        : public ::testing::Test
{

public:
    TestUpdateScheduler()
            : m_loggingSetup(
            std::unique_ptr<UpdateSchedulerImpl::LoggingSetup>(new UpdateSchedulerImpl::LoggingSetup(1)))
              , m_queue(std::make_shared<SchedulerTaskQueue>())
              , m_pluginCallback(std::make_shared<SchedulerPluginCallback>(m_queue))
    {
        Common::ApplicationConfiguration::applicationConfiguration().setData(
                Common::ApplicationConfiguration::SOPHOS_INSTALL, "/installroot"
        );

        Common::ProcessImpl::ProcessFactory::instance().replaceCreator([]() {
                                                                           auto mockProcess = new StrictMock<MockProcess>();
                                                                           return std::unique_ptr<Common::Process::IProcess>(mockProcess);
                                                                       }
        );

    }

    void TearDown() override
    {
        Common::FileSystem::restoreFileSystem();
        Common::ProcessImpl::ProcessFactory::instance().restoreCreator();
    }

    MockFileSystem& setupFileSystemMock()
    {
        using ::testing::Ne;
        auto filesystemMock = new NiceMock<MockFileSystem>();
        auto pointer = filesystemMock;
        // handle the retrieval of machine id from SchedulerProcessor
        EXPECT_CALL(*pointer, isFile("/installroot/base/etc/machine_id.txt")).WillOnce(Return(true));
        EXPECT_CALL(*pointer, readFile("/installroot/base/etc/machine_id.txt")).Times(1).WillOnce(Return("machineId"));

        // required to pass general validation of configuration data
        EXPECT_CALL(*pointer, isDirectory("/installroot")).WillRepeatedly(Return(true));
        EXPECT_CALL(*pointer, isDirectory("/installroot/base/update/cache/primarywarehouse")).WillRepeatedly(Return(true));
        EXPECT_CALL(*pointer, isDirectory("/installroot/base/update/cache/primary")).WillRepeatedly(Return(true));

        EXPECT_CALL(*pointer, exists("/installroot/base/update/certs")).WillRepeatedly(Return(true));
        EXPECT_CALL(*pointer, exists("/installroot/base/update/certs/rootca.crt")).WillRepeatedly(Return(true));
        EXPECT_CALL(*pointer, exists("/installroot/base/update/certs/ps_rootca.crt")).WillRepeatedly(Return(true));

        Common::FileSystem::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));
        return *pointer;
    }


    std::unique_ptr<UpdateSchedulerImpl::LoggingSetup> m_loggingSetup;
    std::shared_ptr<SchedulerTaskQueue> m_queue;
    std::shared_ptr<SchedulerPluginCallback> m_pluginCallback;
};

TEST_F(TestUpdateScheduler, shutdownReceivedShouldStopScheduler) // NOLINT
{
    MockApiBaseServices* api = new StrictMock<MockApiBaseServices>();
    MockAsyncDownloaderRunner* runner = new StrictMock<MockAsyncDownloaderRunner>();
    MockCronSchedulerThread* cron = new StrictMock<MockCronSchedulerThread>();

    EXPECT_CALL(*cron, start());
    EXPECT_CALL(*cron, requestStop());
    EXPECT_CALL(*runner, isRunning()).WillOnce(Return(false));
    setupFileSystemMock();


    UpdateSchedulerImpl::UpdateSchedulerProcessor
    updateScheduler(m_queue, std::unique_ptr<IBaseServiceApi>(api), m_pluginCallback,
                    std::unique_ptr<ICronSchedulerThread>(cron), std::unique_ptr<IAsyncSulDownloaderRunner>(runner));

    std::future<void> schedulerRunHandle = std::async(std::launch::async,
                                                      [&updateScheduler]() { updateScheduler.mainLoop(); }
    );

    m_queue->push(SchedulerTask{SchedulerTask::TaskType::ShutdownReceived, ""});

    schedulerRunHandle.get(); // synchronize stop

}


TEST_F(TestUpdateScheduler, policyConfigureSulDownloaderAndFrequency) // NOLINT
{
    MockApiBaseServices* api = new StrictMock<MockApiBaseServices>();
    MockAsyncDownloaderRunner* runner = new StrictMock<MockAsyncDownloaderRunner>();
    MockCronSchedulerThread* cron = new StrictMock<MockCronSchedulerThread>();

    EXPECT_CALL(*cron, start());
    ICronSchedulerThread::DurationTime time = std::chrono::minutes(50);
    EXPECT_CALL(*cron, setPeriodTime(time));
    EXPECT_CALL(*cron, requestStop());
    EXPECT_CALL(*cron, setScheduledUpdate(false));
    EXPECT_CALL(*runner, isRunning()).WillOnce(Return(false));
    auto& fileSystemMock = setupFileSystemMock();

    UpdateSchedulerImpl::UpdateSchedulerProcessor
    updateScheduler(m_queue, std::unique_ptr<IBaseServiceApi>(api), m_pluginCallback,
                    std::unique_ptr<ICronSchedulerThread>(cron), std::unique_ptr<IAsyncSulDownloaderRunner>(runner));

    EXPECT_CALL(fileSystemMock, writeFile("/installroot/base/update/var/config.json", _));
    EXPECT_CALL(fileSystemMock, isFile("/installroot/base/update/var/report.json")).WillOnce(Return(false));

    std::future<void> schedulerRunHandle = std::async(std::launch::async,
                                                      [&updateScheduler]() { updateScheduler.mainLoop(); }
    );

    m_queue->push(SchedulerTask{SchedulerTask::TaskType::Policy, updatePolicyWithProxy});

    m_queue->push(SchedulerTask{SchedulerTask::TaskType::ShutdownReceived, ""});
    schedulerRunHandle.get(); // synchronize stop

}


TEST_F(TestUpdateScheduler, policyWithCacheConfigureSulDownloaderAndFrequency) // NOLINT
{
    MockApiBaseServices* api = new StrictMock<MockApiBaseServices>();
    MockAsyncDownloaderRunner* runner = new StrictMock<MockAsyncDownloaderRunner>();
    MockCronSchedulerThread* cron = new StrictMock<MockCronSchedulerThread>();

    EXPECT_CALL(*cron, start());
    ICronSchedulerThread::DurationTime time = std::chrono::minutes(50);
    EXPECT_CALL(*cron, setPeriodTime(time));
    EXPECT_CALL(*cron, requestStop());
    EXPECT_CALL(*cron, setScheduledUpdate(false));
    EXPECT_CALL(*runner, isRunning()).WillOnce(Return(false));
    auto& fileSystemMock = setupFileSystemMock();

    UpdateSchedulerImpl::UpdateSchedulerProcessor
    updateScheduler(m_queue, std::unique_ptr<IBaseServiceApi>(api), m_pluginCallback,
                    std::unique_ptr<ICronSchedulerThread>(cron), std::unique_ptr<IAsyncSulDownloaderRunner>(runner));

    EXPECT_CALL(fileSystemMock, isFile("/installroot/base/update/var/report.json")).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, writeFile("/installroot/base/update/var/config.json", _));
    EXPECT_CALL(fileSystemMock, writeFile("/installroot/base/update/certs/cache_certificates.crt", _));
    EXPECT_CALL(fileSystemMock, exists("/installroot/base/update/certs/cache_certificates.crt")).WillOnce(Return(true));

    std::future<void> schedulerRunHandle = std::async(std::launch::async,
                                                      [&updateScheduler]() { updateScheduler.mainLoop(); }
    );

    m_queue->push(SchedulerTask{SchedulerTask::TaskType::Policy, updatePolicyWithCache});

    m_queue->push(SchedulerTask{SchedulerTask::TaskType::ShutdownReceived, ""});
    schedulerRunHandle.get(); // synchronize stop

}

TEST_F(TestUpdateScheduler, handleActionNow) // NOLINT
{
    MockApiBaseServices* api = new StrictMock<MockApiBaseServices>();
    MockAsyncDownloaderRunner* runner = new StrictMock<MockAsyncDownloaderRunner>();
    MockCronSchedulerThread* cron = new StrictMock<MockCronSchedulerThread>();

    EXPECT_CALL(*cron, start());
    ICronSchedulerThread::DurationTime time = std::chrono::minutes(50);
    EXPECT_CALL(*cron, setPeriodTime(time));
    // update now restart the chron time
    EXPECT_CALL(*cron, reset());
    EXPECT_CALL(*cron, requestStop());
    EXPECT_CALL(*cron, setScheduledUpdate(false));
    EXPECT_CALL(*runner, isRunning()).WillOnce(Return(false)).WillOnce(Return(false));
    EXPECT_CALL(*runner, triggerSulDownloader());
    auto& fileSystemMock = setupFileSystemMock();

    UpdateSchedulerImpl::UpdateSchedulerProcessor
    updateScheduler(m_queue, std::unique_ptr<IBaseServiceApi>(api), m_pluginCallback,
                    std::unique_ptr<ICronSchedulerThread>(cron), std::unique_ptr<IAsyncSulDownloaderRunner>(runner));

    std::string reportPath = Common::ApplicationConfiguration::applicationPathManager()
            .getSulDownloaderReportGeneratedFilePath();

    EXPECT_CALL(fileSystemMock, writeFile("/installroot/base/update/var/config.json", _));
    EXPECT_CALL(fileSystemMock, isFile("/installroot/base/update/var/config.json")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isFile(HasSubstr("/installroot/base/update/var/report")))
            .WillOnce(Return(false)) // after policy no report exist
            .WillOnce(Return(true))  // after the first report is created
            .WillOnce(Return(false)); // after checking if it is safe to move file
    EXPECT_CALL(fileSystemMock, moveFile(reportPath, _));
    // the real name would be different as the file has been moved. But, ignoring here.
    std::vector<std::string> files{reportPath, "/installroot/base/update/var/config.json"};
    std::vector<std::string> noReportFiles{"/installroot/base/update/var/config.json"};
    EXPECT_CALL(fileSystemMock, listFiles("/installroot/base/update/var"))
            .WillOnce(Return(noReportFiles)) // after policy it process current reports (empty)
            .WillOnce(Return(files)); // report generated by suldownloader
    EXPECT_CALL(fileSystemMock, readFile(reportPath)).WillOnce(Return(downloadReport));

    EXPECT_CALL(*api, sendEvent("ALC", _));
    EXPECT_CALL(*api, sendStatus("ALC", _, _));

    std::future<void> schedulerRunHandle = std::async(std::launch::async,
                                                      [&updateScheduler]() { updateScheduler.mainLoop(); }
    );

    m_queue->push(SchedulerTask{SchedulerTask::TaskType::Policy, updatePolicyWithProxy});

    m_queue->push(SchedulerTask{SchedulerTask::TaskType::UpdateNow, updateAction});

    // on success... runner will add task to the queue
    m_queue->push(SchedulerTask{SchedulerTask::TaskType::SulDownloaderFinished, ""});


    m_queue->push(SchedulerTask{SchedulerTask::TaskType::ShutdownReceived, ""});
    schedulerRunHandle.get(); // synchronize stop

}

TEST_F(TestUpdateScheduler, invalidPolicyWillNotCreateAConfig) // NOLINT
{
    MockApiBaseServices* api = new StrictMock<MockApiBaseServices>();
    MockAsyncDownloaderRunner* runner = new StrictMock<MockAsyncDownloaderRunner>();
    MockCronSchedulerThread* cron = new StrictMock<MockCronSchedulerThread>();

    EXPECT_CALL(*cron, start());
    ICronSchedulerThread::DurationTime time = std::chrono::minutes(50);

    EXPECT_CALL(*cron, requestStop());
    EXPECT_CALL(*runner, isRunning()).WillOnce(Return(false));

    auto& fileSystemMock = setupFileSystemMock();

    UpdateSchedulerImpl::UpdateSchedulerProcessor
    updateScheduler(m_queue, std::unique_ptr<IBaseServiceApi>(api), m_pluginCallback,
                    std::unique_ptr<ICronSchedulerThread>(cron), std::unique_ptr<IAsyncSulDownloaderRunner>(runner));

    // no config will be created when an invalid policy is given
    EXPECT_CALL(*cron, setPeriodTime(time)).Times(0);
    EXPECT_CALL(fileSystemMock, writeFile("/installroot/base/update/var/config.json", _)).Times(0);


    std::future<void> schedulerRunHandle = std::async(std::launch::async,
                                                      [&updateScheduler]() { updateScheduler.mainLoop(); }
    );


    std::string invalidPolicyEmptyUserName = Common::UtilityImpl::StringUtils::orderedStringReplace(updatePolicyWithProxy,{{"UserPassword=\"54m5ung\"","UserPassword=\"\""}} );

    m_queue->push(SchedulerTask{SchedulerTask::TaskType::Policy, invalidPolicyEmptyUserName});

    std::string invalidPolicyPeriod = Common::UtilityImpl::StringUtils::orderedStringReplace(updatePolicyWithProxy,
            {{R"sophos(SchedEnable="true" Frequency="50")sophos", R"sophos(SchedEnable="true" Frequency="50000000")sophos" }} );

    m_queue->push(SchedulerTask{SchedulerTask::TaskType::Policy, invalidPolicyPeriod});

    m_queue->push(SchedulerTask{SchedulerTask::TaskType::ShutdownReceived, ""});
    schedulerRunHandle.get(); // synchronize stop

}

TEST_F(TestUpdateScheduler, scheduledUpdatePolicyWillConfigureSchedule) // NOLINT
{
    MockApiBaseServices* api = new StrictMock<MockApiBaseServices>();
    MockAsyncDownloaderRunner* runner = new StrictMock<MockAsyncDownloaderRunner>();
    MockCronSchedulerThread* cron = new StrictMock<MockCronSchedulerThread>();

    EXPECT_CALL(*cron, start());
    ICronSchedulerThread::DurationTime time = std::chrono::minutes(1);
    EXPECT_CALL(*cron, setPeriodTime(time));
    EXPECT_CALL(*cron, requestStop());
    EXPECT_CALL(*cron, setScheduledUpdate(true));

    std::tm actualTime;
    EXPECT_CALL(*cron, setScheduledUpdateTime(_)).WillOnce(SaveArg<0>(&actualTime));

    EXPECT_CALL(*runner, isRunning()).WillOnce(Return(false));
    auto& fileSystemMock = setupFileSystemMock();

    UpdateSchedulerImpl::UpdateSchedulerProcessor
    updateScheduler(m_queue, std::unique_ptr<IBaseServiceApi>(api), m_pluginCallback,
                    std::unique_ptr<ICronSchedulerThread>(cron), std::unique_ptr<IAsyncSulDownloaderRunner>(runner));

    EXPECT_CALL(fileSystemMock, writeFile("/installroot/base/update/var/config.json", _));
    EXPECT_CALL(fileSystemMock, isFile("/installroot/base/update/var/report.json")).WillOnce(Return(false));

    std::future<void> schedulerRunHandle = std::async(std::launch::async,
                                                      [&updateScheduler]() { updateScheduler.mainLoop(); }
    );

    m_queue->push(SchedulerTask{SchedulerTask::TaskType::Policy, updatePolicyWithScheduledUpdate});

    m_queue->push(SchedulerTask{SchedulerTask::TaskType::ShutdownReceived, ""});
    schedulerRunHandle.get(); // synchronize stop

    // Saturday(6th Weekday), 9:41
    EXPECT_EQ(actualTime.tm_hour, 9);
    EXPECT_EQ(actualTime.tm_min, 41);
    EXPECT_EQ(actualTime.tm_wday, 6);
}

TEST_F(TestUpdateScheduler, badScheduledUpdatePolicyWillNotConfigureSchedule) // NOLINT
{
    MockApiBaseServices* api = new StrictMock<MockApiBaseServices>();
    MockAsyncDownloaderRunner* runner = new StrictMock<MockAsyncDownloaderRunner>();
    MockCronSchedulerThread* cron = new StrictMock<MockCronSchedulerThread>();

    EXPECT_CALL(*cron, start());
    ICronSchedulerThread::DurationTime time = std::chrono::minutes(40);
    EXPECT_CALL(*cron, setPeriodTime(time)).Times(4);
    EXPECT_CALL(*cron, requestStop());
    EXPECT_CALL(*cron, setScheduledUpdate(false)).Times(4);

    EXPECT_CALL(*runner, isRunning()).WillOnce(Return(false));
    auto& fileSystemMock = setupFileSystemMock();

    UpdateSchedulerImpl::UpdateSchedulerProcessor
    updateScheduler(m_queue, std::unique_ptr<IBaseServiceApi>(api), m_pluginCallback,
                    std::unique_ptr<ICronSchedulerThread>(cron), std::unique_ptr<IAsyncSulDownloaderRunner>(runner));

    EXPECT_CALL(fileSystemMock, writeFile("/installroot/base/update/var/config.json", _)).Times(4);
    EXPECT_CALL(fileSystemMock, isFile("/installroot/base/update/var/report.json")).WillOnce(Return(false));

    std::future<void> schedulerRunHandle = std::async(std::launch::async,
                                                      [&updateScheduler]() { updateScheduler.mainLoop(); }
    );

    std::string invalidPolicyScheduleDay = Common::UtilityImpl::StringUtils::orderedStringReplace(updatePolicyWithScheduledUpdate,
            {{R"sophos(Day="Saturday" Time="09:41:00")sophos", R"sophos(Day="Blahday" Time="09:41:00")sophos" }} );
    std::string invalidPolicyScheduleTime = Common::UtilityImpl::StringUtils::orderedStringReplace(updatePolicyWithScheduledUpdate,
            {{R"sophos(Day="Saturday" Time="09:41:00")sophos", R"sophos(Day="Saturday" Time="24:00:00")sophos" }} );
    std::string invalidPolicyScheduleTimeType = Common::UtilityImpl::StringUtils::orderedStringReplace(updatePolicyWithScheduledUpdate,
            {{R"sophos(Day="Saturday" Time="09:41:00")sophos", R"sophos(Day="Monday" Time="a:a:a")sophos" }} );
    std::string invalidPolicyScheduleDayType = Common::UtilityImpl::StringUtils::orderedStringReplace(updatePolicyWithScheduledUpdate,
            {{R"sophos(Day="Saturday" Time="09:41:00")sophos", R"sophos(Day="0" Time="11:11:11")sophos" }} );

    m_queue->push(SchedulerTask{SchedulerTask::TaskType::Policy, invalidPolicyScheduleDay});
    m_queue->push(SchedulerTask{SchedulerTask::TaskType::Policy, invalidPolicyScheduleTime});
    m_queue->push(SchedulerTask{SchedulerTask::TaskType::Policy, invalidPolicyScheduleTimeType});
    m_queue->push(SchedulerTask{SchedulerTask::TaskType::Policy, invalidPolicyScheduleDayType});

    m_queue->push(SchedulerTask{SchedulerTask::TaskType::ShutdownReceived, ""});
    schedulerRunHandle.get(); // synchronize stop
}
