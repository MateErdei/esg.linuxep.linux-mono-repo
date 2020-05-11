/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "MockAsyncDownloaderRunner.h"
#include "MockCronSchedulerThread.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystemImpl/FileSystemImpl.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/ProcessImpl/ProcessImpl.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <UpdateSchedulerImpl/Logger.h>
#include <UpdateSchedulerImpl/UpdateSchedulerProcessor.h>
#include <gmock/gmock-matchers.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/MockApiBaseServices.h>
#include <tests/Common/Helpers/MockFileSystem.h>
#include <tests/Common/ProcessImpl/MockProcess.h>

#include <future>

namespace
{
    std::string updatePolicyWithProxy{ R"sophos(<?xml version="1.0"?>
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
    <cloud_subscription RigidName="ServerProtectionLinux-Base" Tag="RECOMMENDED" BaseVersion="10"/>
    <cloud_subscriptions>
      <subscription Id="Base" RigidName="ServerProtectionLinux-Base" Tag="RECOMMENDED" BaseVersion="10"/>
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
)sophos" };
    std::string updatePolicyWithCache{ R"sophos(<?xml version="1.0"?>
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
    <cloud_subscription RigidName="ServerProtectionLinux-Base" Tag="RECOMMENDED" BaseVersion="10"/>
    <cloud_subscriptions>
      <subscription Id="Base" RigidName="ServerProtectionLinux-Base" Tag="RECOMMENDED" BaseVersion="10"/>
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
)sophos" };

    std::string updateAction{ R"sophos(<?xml version='1.0'?><action type="sophos.mgt.action.ALCForceUpdate"/>)sophos" };
    std::string downloadReport{ R"sophos({ "startTime": "20180810 10:00:00",
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
)sophos" };

    std::string downloadReportUpgraded =
            R"({
 "startTime": "20200423 160452",
 "finishTime": "20200423 160514",
 "syncTime": "20200423 160514",
 "status": "SUCCESS",
 "sulError": "",
 "errorDescription": "",
 "urlSource": "http://dci.sophosupd.com/update",
 "products": [
  {
   "rigidName": "ServerProtectionLinux-Base",
   "productName": "Sophos Linux Base",
   "downloadVersion": "1.1.1.15",
   "errorDescription": "",
   "productStatus": "UPGRADED"
  },
  {
   "rigidName": "ServerProtectionLinux-Plugin-EDR",
   "productName": "Sophos Live Discover Plugin",
   "downloadVersion": "1.0.1.29",
   "errorDescription": "",
   "productStatus": "UPGRADED"
  },
  {
   "rigidName": "ServerProtectionLinux-Plugin-MDR",
   "productName": "Sophos Server Protection for Linux MTR",
   "downloadVersion": "1.0.4.13",
   "errorDescription": "",
   "productStatus": "UPGRADED"
  }
 ],
 "warehouseComponents": [
  {
   "rigidName": "ServerProtectionLinux-Base",
   "productName": "Sophos Linux Base",
   "installedVersion": "1.1.1.15"
  },
  {
   "rigidName": "ServerProtectionLinux-Plugin-EDR",
   "productName": "Sophos Live Discover Plugin",
   "installedVersion": "1.0.1.29"
  },
  {
   "rigidName": "ServerProtectionLinux-MDR-osquery-Component",
   "productName": "Sophos Query Component",
   "installedVersion": "4.3.0"
  },
  {
   "rigidName": "ServerProtectionLinux-MDR-Control-Component",
   "productName": "Sophos Managed Threat Response plug-in",
   "installedVersion": "1.0.4.20"
  },
  {
   "rigidName": "ServerProtectionLinux-MDR-DBOS-Component",
   "productName": "Sophos Managed Threat Response for Linux",
   "installedVersion": "1.0.3.15"
  }
 ]
}
)";

    std::string downloadReportUpToDate =
            R"({
 "startTime": "20200424 084004",
 "finishTime": "20200424 084005",
 "syncTime": "20200424 084005",
 "status": "SUCCESS",
 "sulError": "",
 "errorDescription": "",
 "urlSource": "http://dci.sophosupd.com/update",
 "products": [
  {
   "rigidName": "ServerProtectionLinux-Base",
   "productName": "Sophos Linux Base",
   "downloadVersion": "1.1.1.15",
   "errorDescription": "",
   "productStatus": "UPTODATE"
  },
  {
   "rigidName": "ServerProtectionLinux-Plugin-EDR",
   "productName": "Sophos Live Discover Plugin",
   "downloadVersion": "1.0.1.29",
   "errorDescription": "",
   "productStatus": "UPTODATE"
  },
  {
   "rigidName": "ServerProtectionLinux-Plugin-MDR",
   "productName": "Sophos Server Protection for Linux MTR",
   "downloadVersion": "1.0.4.13",
   "errorDescription": "",
   "productStatus": "UPTODATE"
  }
 ],
 "warehouseComponents": [
  {
   "rigidName": "ServerProtectionLinux-Base",
   "productName": "Sophos Linux Base",
   "installedVersion": "1.1.1.15"
  },
  {
   "rigidName": "ServerProtectionLinux-Plugin-EDR",
   "productName": "Sophos Live Discover Plugin",
   "installedVersion": "1.0.1.29"
  },
  {
   "rigidName": "ServerProtectionLinux-MDR-osquery-Component",
   "productName": "Sophos Query Component",
   "installedVersion": "4.3.0"
  },
  {
   "rigidName": "ServerProtectionLinux-MDR-Control-Component",
   "productName": "Sophos Managed Threat Response plug-in",
   "installedVersion": "1.0.4.20"
  },
  {
   "rigidName": "ServerProtectionLinux-MDR-DBOS-Component",
   "productName": "Sophos Managed Threat Response for Linux",
   "installedVersion": "1.0.3.15"
  }
 ]
}
)";

} // namespace

static const std::string updatePolicyWithScheduledUpdate{ R"sophos(<?xml version="1.0"?>
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
    <cloud_subscription RigidName="ServerProtectionLinux-Base" Tag="RECOMMENDED" BaseVersion="10"/>
    <cloud_subscriptions>
      <subscription Id="Base" RigidName="ServerProtectionLinux-Base" Tag="RECOMMENDED" BaseVersion="10"/>
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
)sophos" };

using namespace UpdateSchedulerImpl;
using namespace UpdateScheduler;
using namespace Common::PluginApi;

class TestUpdateScheduler : public ::testing::Test
{
public:
    TestUpdateScheduler() :
        m_queue(std::make_shared<SchedulerTaskQueue>()),
        m_pluginCallback(std::make_shared<SchedulerPluginCallback>(m_queue))
    {
        Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL, "/installroot");

        Common::ProcessImpl::ProcessFactory::instance().replaceCreator([]() {
            auto mockProcess = new StrictMock<MockProcess>();
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        });
    }

    MockApiBaseServices* api;
    MockAsyncDownloaderRunner* runner;
    MockCronSchedulerThread* cron;

    void SetUp() override
    {
        api = new StrictMock<MockApiBaseServices>();
        runner = new StrictMock<MockAsyncDownloaderRunner>();
        cron = new StrictMock<MockCronSchedulerThread>();

        EXPECT_CALL(*api, requestPolicies(_));
        EXPECT_CALL(*cron, start());
    }
    void TearDown() override
    {
        Tests::restoreFileSystem();
        Common::ProcessImpl::ProcessFactory::instance().restoreCreator();
    }

    MockFileSystem& setupFileSystemMock()
    {
        using ::testing::Ne;
        auto filesystemMock = new NiceMock<MockFileSystem>();
        auto pointer = filesystemMock;
        // handle the retrieval of machine id from SchedulerProcessor
        EXPECT_CALL(*pointer, isFile("/installroot/base/etc/machine_id.txt")).WillOnce(Return(true));
        EXPECT_CALL(*pointer, isFile("/installroot/base/update/var/sophos_alias.txt")).WillRepeatedly(Return(false));
        EXPECT_CALL(*pointer, isFile("/installroot/base/update/var/previous_update_config.json")).WillRepeatedly(Return(false));
        EXPECT_CALL(*pointer, readFile("/installroot/base/etc/machine_id.txt")).Times(1).WillOnce(Return("machineId"));

        // required to pass general validation of configuration data
        EXPECT_CALL(*pointer, isDirectory("/installroot")).WillRepeatedly(Return(true));
        EXPECT_CALL(*pointer, isDirectory("/installroot/base/update/cache/primarywarehouse"))
            .WillRepeatedly(Return(true));
        EXPECT_CALL(*pointer, isDirectory("/installroot/base/update/cache/primary")).WillRepeatedly(Return(true));

        EXPECT_CALL(*pointer, exists("/installroot/base/update/certs")).WillRepeatedly(Return(true));
        EXPECT_CALL(*pointer, exists("/installroot/base/update/certs/rootca.crt")).WillRepeatedly(Return(true));
        EXPECT_CALL(*pointer, exists("/installroot/base/update/certs/ps_rootca.crt")).WillRepeatedly(Return(true));

        Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));
        return *pointer;
    }

    Common::Logging::ConsoleLoggingSetup m_loggingSetup;
    std::shared_ptr<SchedulerTaskQueue> m_queue;
    std::shared_ptr<SchedulerPluginCallback> m_pluginCallback;

};

TEST_F(TestUpdateScheduler, shutdownReceivedShouldStopScheduler) // NOLINT
{
    EXPECT_CALL(*cron, requestStop());
    EXPECT_CALL(*runner, isRunning()).WillOnce(Return(false));
    setupFileSystemMock();

    UpdateSchedulerImpl::UpdateSchedulerProcessor updateScheduler(
        m_queue,
        std::unique_ptr<IBaseServiceApi>(api),
        m_pluginCallback,
        std::unique_ptr<ICronSchedulerThread>(cron),
        std::unique_ptr<IAsyncSulDownloaderRunner>(runner));

    std::future<void> schedulerRunHandle =
        std::async(std::launch::async, [&updateScheduler]() { updateScheduler.mainLoop(); });

    m_queue->push(SchedulerTask{ SchedulerTask::TaskType::ShutdownReceived, "" });

    schedulerRunHandle.get(); // synchronize stop
}

TEST_F(TestUpdateScheduler, policyConfigureSulDownloaderAndFrequency) // NOLINT
{
    ICronSchedulerThread::DurationTime time = std::chrono::minutes(50);
    EXPECT_CALL(*cron, setPeriodTime(time));
    EXPECT_CALL(*cron, requestStop());

    ScheduledUpdate scheduledUpdate;
    EXPECT_CALL(*cron, setScheduledUpdate(_)).WillOnce(SaveArg<0>(&scheduledUpdate));

    EXPECT_CALL(*runner, isRunning()).WillOnce(Return(false));
    auto& fileSystemMock = setupFileSystemMock();

    UpdateSchedulerImpl::UpdateSchedulerProcessor updateScheduler(
        m_queue,
        std::unique_ptr<IBaseServiceApi>(api),
        m_pluginCallback,
        std::unique_ptr<ICronSchedulerThread>(cron),
        std::unique_ptr<IAsyncSulDownloaderRunner>(runner));

    EXPECT_CALL(fileSystemMock, writeFile("/installroot/base/update/var/update_config.json", _));
    EXPECT_CALL(fileSystemMock, isFile("/installroot/base/update/var/update_report.json")).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, isFile("/installroot/base/update/var/update_config.json")).WillOnce(Return(true));

    std::future<void> schedulerRunHandle =
        std::async(std::launch::async, [&updateScheduler]() { updateScheduler.mainLoop(); });

    m_queue->push(SchedulerTask{ SchedulerTask::TaskType::Policy, updatePolicyWithProxy });

    m_queue->push(SchedulerTask{ SchedulerTask::TaskType::ShutdownReceived, "" });
    schedulerRunHandle.get(); // synchronize stop

    EXPECT_EQ(scheduledUpdate.getEnabled(), false);
}

TEST_F(TestUpdateScheduler, policyWithCacheConfigureSulDownloaderAndFrequency) // NOLINT
{
    ICronSchedulerThread::DurationTime time = std::chrono::minutes(50);
    EXPECT_CALL(*cron, setPeriodTime(time));
    EXPECT_CALL(*cron, requestStop());

    ScheduledUpdate scheduledUpdate;
    EXPECT_CALL(*cron, setScheduledUpdate(_)).WillOnce(SaveArg<0>(&scheduledUpdate));

    EXPECT_CALL(*runner, isRunning()).WillOnce(Return(false));
    auto& fileSystemMock = setupFileSystemMock();

    UpdateSchedulerImpl::UpdateSchedulerProcessor updateScheduler(
        m_queue,
        std::unique_ptr<IBaseServiceApi>(api),
        m_pluginCallback,
        std::unique_ptr<ICronSchedulerThread>(cron),
        std::unique_ptr<IAsyncSulDownloaderRunner>(runner));

    EXPECT_CALL(fileSystemMock, isFile("/installroot/base/update/var/update_report.json")).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, writeFile("/installroot/base/update/var/update_config.json", _));
    EXPECT_CALL(fileSystemMock, writeFile("/installroot/base/update/certs/cache_certificates.crt", _));
    //    EXPECT_CALL(fileSystemMock,
    //    exists("/installroot/base/update/certs/cache_certificates.crt")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isFile("/installroot/base/update/var/update_config.json")).WillOnce(Return(true));

    std::future<void> schedulerRunHandle =
        std::async(std::launch::async, [&updateScheduler]() { updateScheduler.mainLoop(); });

    m_queue->push(SchedulerTask{ SchedulerTask::TaskType::Policy, updatePolicyWithCache });

    m_queue->push(SchedulerTask{ SchedulerTask::TaskType::ShutdownReceived, "" });
    schedulerRunHandle.get(); // synchronize stop

    EXPECT_EQ(scheduledUpdate.getEnabled(), false);
}

TEST_F(TestUpdateScheduler, handleActionNow) // NOLINT
{
    ICronSchedulerThread::DurationTime time = std::chrono::minutes(50);
    EXPECT_CALL(*cron, setPeriodTime(time));
    // update now restart the chron time
    EXPECT_CALL(*cron, reset());
    EXPECT_CALL(*cron, requestStop());

    ScheduledUpdate scheduledUpdate;
    EXPECT_CALL(*cron, setScheduledUpdate(_)).WillOnce(SaveArg<0>(&scheduledUpdate));

    EXPECT_CALL(*runner, isRunning()).WillOnce(Return(false)).WillOnce(Return(false));
    EXPECT_CALL(*runner, triggerSulDownloader());
    auto& fileSystemMock = setupFileSystemMock();

    UpdateSchedulerImpl::UpdateSchedulerProcessor updateScheduler(
        m_queue,
        std::unique_ptr<IBaseServiceApi>(api),
        m_pluginCallback,
        std::unique_ptr<ICronSchedulerThread>(cron),
        std::unique_ptr<IAsyncSulDownloaderRunner>(runner));

    std::string reportPath =
        Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderReportGeneratedFilePath();

    EXPECT_CALL(fileSystemMock, writeFile("/installroot/base/update/var/update_config.json", _));
    EXPECT_CALL(fileSystemMock, isFile("/installroot/base/update/var/update_config.json"))
        .Times(2)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, isFile(HasSubstr("/installroot/base/update/var/update_report")))
        .WillOnce(Return(false))  // after policy no report exist
        .WillOnce(Return(true))   // after the first report is created
        .WillOnce(Return(false)); // after checking if it is safe to move file
    EXPECT_CALL(fileSystemMock, moveFile(reportPath, _));
    // the real name would be different as the file has been moved. But, ignoring here.
    std::vector<std::string> files{ reportPath, "/installroot/base/update/var/update_config.json" };
    std::vector<std::string> noReportFiles{ "/installroot/base/update/var/update_config.json" };
    EXPECT_CALL(fileSystemMock, listFiles("/installroot/base/update/var"))
        .WillOnce(Return(files)); // report generated by suldownloader

    EXPECT_CALL(fileSystemMock, listFiles("/installroot/base/update/var"))
        .Times(2)
        .WillRepeatedly(Return(noReportFiles)) // after policy it process current reports (empty)
        .RetiresOnSaturation();

    EXPECT_CALL(fileSystemMock, readFile(reportPath)).WillOnce(Return(downloadReport));

    EXPECT_CALL(fileSystemMock, isFile("/installroot/base/update/var/processedReports/update_report.json")).WillOnce(Return(false));

    EXPECT_CALL(fileSystemMock, writeFile("/installroot/base/update/var/processedReports/update_report.json", _));
    EXPECT_CALL(*api, sendEvent("ALC", _));
    EXPECT_CALL(*api, sendStatus("ALC", _, _));

    std::future<void> schedulerRunHandle =
        std::async(std::launch::async, [&updateScheduler]() { updateScheduler.mainLoop(); });

    m_queue->push(SchedulerTask{ SchedulerTask::TaskType::Policy, updatePolicyWithProxy });

    m_queue->push(SchedulerTask{ SchedulerTask::TaskType::UpdateNow, updateAction });

    // on success... runner will add task to the queue
    m_queue->push(SchedulerTask{ SchedulerTask::TaskType::SulDownloaderFinished, "" });

    m_queue->push(SchedulerTask{ SchedulerTask::TaskType::ShutdownReceived, "" });
    schedulerRunHandle.get(); // synchronize stop

    EXPECT_EQ(scheduledUpdate.getEnabled(), false);
}

TEST_F(TestUpdateScheduler, UpdateUnprocessedReportUpgradeReportResultsInSendingEvent) // NOLINT
{
    ICronSchedulerThread::DurationTime time = std::chrono::minutes(50);
    EXPECT_CALL(*cron, setPeriodTime(time));
    // update now restart the chron time
    EXPECT_CALL(*cron, reset());
    EXPECT_CALL(*cron, requestStop());

    ScheduledUpdate scheduledUpdate;
    EXPECT_CALL(*cron, setScheduledUpdate(_)).WillOnce(SaveArg<0>(&scheduledUpdate));

    EXPECT_CALL(*runner, isRunning()).WillOnce(Return(false)).WillOnce(Return(false));
    EXPECT_CALL(*runner, triggerSulDownloader());
    auto& fileSystemMock = setupFileSystemMock();

    UpdateSchedulerImpl::UpdateSchedulerProcessor updateScheduler(
            m_queue,
            std::unique_ptr<IBaseServiceApi>(api),
            m_pluginCallback,
            std::unique_ptr<ICronSchedulerThread>(cron),
            std::unique_ptr<IAsyncSulDownloaderRunner>(runner));

    std::string reportPath =
            Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderReportGeneratedFilePath();

    EXPECT_CALL(fileSystemMock, writeFile("/installroot/base/update/var/update_config.json", _));
    EXPECT_CALL(fileSystemMock, isFile("/installroot/base/update/var/update_config.json"))
            .Times(2)
            .WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, isFile(HasSubstr("/installroot/base/update/var/update_report")))
            .WillOnce(Return(false))  // after policy no report exist
            .WillOnce(Return(true))   // after the first report is created
            .WillOnce(Return(false)); // after checking if it is safe to move file
    EXPECT_CALL(fileSystemMock, moveFile(reportPath, _));
    // the real name would be different as the file has been moved. But, ignoring here.
    std::vector<std::string> files{ reportPath, "/installroot/base/update/var/update_config.json",
                                    "/installroot/base/update/var/update_report_1.json",
                                    "/installroot/base/update/var/update_report_2.json"};

    std::vector<std::string> noReportFiles{ "/installroot/base/update/var/update_config.json"};

    EXPECT_CALL(fileSystemMock, listFiles("/installroot/base/update/var"))
            .WillOnce(Return(files)); // report generated by suldownloader

    EXPECT_CALL(fileSystemMock, listFiles("/installroot/base/update/var"))
            .Times(2)
            .WillRepeatedly(Return(noReportFiles)) // after policy it process current reports (empty)
            .RetiresOnSaturation();

    EXPECT_CALL(fileSystemMock, readFile(reportPath)).WillOnce(Return(downloadReportUpToDate));
    EXPECT_CALL(fileSystemMock, readFile("/installroot/base/update/var/update_report_1.json")).WillOnce(Return(downloadReport));
    EXPECT_CALL(fileSystemMock, readFile("/installroot/base/update/var/update_report_2.json")).WillOnce(Return(downloadReport));

    EXPECT_CALL(fileSystemMock, isFile("/installroot/base/update/var/processedReports/update_report.json")).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, isFile("/installroot/base/update/var/processedReports/update_report_1.json")).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, isFile("/installroot/base/update/var/processedReports/update_report_2.json")).WillOnce(Return(false));

    EXPECT_CALL(fileSystemMock, removeFilesInDirectory("/installroot/base/update/var/processedReports")).Times(1);
    EXPECT_CALL(fileSystemMock, isDirectory("/installroot/base/update/var/processedReports")).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, listFiles("/installroot/base/update/var/processedReports")).WillRepeatedly(Return(std::vector<Path>()));

    EXPECT_CALL(fileSystemMock, writeFile("/installroot/base/update/var/processedReports/update_report.json", _)).Times(1);
    EXPECT_CALL(fileSystemMock, writeFile("/installroot/base/update/var/processedReports/update_report_2.json", _)).Times(1);
    EXPECT_CALL(fileSystemMock, copyFile("/installroot/base/update/var/update_config.json", _)).Times(1);

    EXPECT_CALL(*api, sendEvent("ALC", _));
    EXPECT_CALL(*api, sendStatus("ALC", _, _));

    std::future<void> schedulerRunHandle =
            std::async(std::launch::async, [&updateScheduler]() { updateScheduler.mainLoop(); });

    m_queue->push(SchedulerTask{ SchedulerTask::TaskType::Policy, updatePolicyWithProxy });

    m_queue->push(SchedulerTask{ SchedulerTask::TaskType::UpdateNow, updateAction });

    // on success... runner will add task to the queue
    m_queue->push(SchedulerTask{ SchedulerTask::TaskType::SulDownloaderFinished, "" });

    m_queue->push(SchedulerTask{ SchedulerTask::TaskType::ShutdownReceived, "" });
    schedulerRunHandle.get(); // synchronize stop

    EXPECT_EQ(scheduledUpdate.getEnabled(), false);
}

TEST_F(TestUpdateScheduler, UppdateUnprocessedReportComparingProcessedReportResultsInNotSendingEvent) // NOLINT
{
    ICronSchedulerThread::DurationTime time = std::chrono::minutes(50);
    EXPECT_CALL(*cron, setPeriodTime(time));
    // update now restart the chron time
    EXPECT_CALL(*cron, reset());
    EXPECT_CALL(*cron, requestStop());

    ScheduledUpdate scheduledUpdate;
    EXPECT_CALL(*cron, setScheduledUpdate(_)).WillOnce(SaveArg<0>(&scheduledUpdate));

    EXPECT_CALL(*runner, isRunning()).WillOnce(Return(false)).WillOnce(Return(false));
    EXPECT_CALL(*runner, triggerSulDownloader());
    auto& fileSystemMock = setupFileSystemMock();

    UpdateSchedulerImpl::UpdateSchedulerProcessor updateScheduler(
            m_queue,
            std::unique_ptr<IBaseServiceApi>(api),
            m_pluginCallback,
            std::unique_ptr<ICronSchedulerThread>(cron),
            std::unique_ptr<IAsyncSulDownloaderRunner>(runner));

    std::string reportPath =
            Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderReportGeneratedFilePath();

    EXPECT_CALL(fileSystemMock, writeFile("/installroot/base/update/var/update_config.json", _));
    EXPECT_CALL(fileSystemMock, isFile("/installroot/base/update/var/update_config.json"))
            .Times(2)
            .WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, isFile(HasSubstr("/installroot/base/update/var/update_report")))
            .WillOnce(Return(false))  // after policy no report exist
            .WillOnce(Return(true))   // after the first report is created
            .WillOnce(Return(false)); // after checking if it is safe to move file
    EXPECT_CALL(fileSystemMock, moveFile(reportPath, _));
    // the real name would be different as the file has been moved. But, ignoring here.
    std::vector<std::string> files{ reportPath, "/installroot/base/update/var/update_config.json",
                                    "/installroot/base/update/var/update_report_1.json",
                                    "/installroot/base/update/var/update_report_2.json"};

    std::vector<std::string> noReportFiles{ "/installroot/base/update/var/update_config.json"};

    EXPECT_CALL(fileSystemMock, listFiles("/installroot/base/update/var"))
            .WillOnce(Return(files)); // report generated by suldownloader

    EXPECT_CALL(fileSystemMock, listFiles("/installroot/base/update/var"))
            .Times(2)
            .WillRepeatedly(Return(noReportFiles)) // after policy it process current reports (empty)
            .RetiresOnSaturation();

    EXPECT_CALL(fileSystemMock, readFile(reportPath)).WillOnce(Return(downloadReportUpToDate));
    EXPECT_CALL(fileSystemMock, readFile("/installroot/base/update/var/update_report_1.json")).WillOnce(Return(downloadReportUpToDate));
    EXPECT_CALL(fileSystemMock, readFile("/installroot/base/update/var/update_report_2.json")).WillOnce(Return(downloadReportUpgraded));

    EXPECT_CALL(fileSystemMock, isFile("/installroot/base/update/var/processedReports/update_report.json")).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, isFile("/installroot/base/update/var/processedReports/update_report_1.json")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isFile("/installroot/base/update/var/processedReports/update_report_2.json")).WillOnce(Return(true));

    std::vector<Path> listProcessFiles {"/installroot/base/update/var/processedReports/update_report_1.json", "/installroot/base/update/var/processedReports/update_report_2.json"};

    EXPECT_CALL(fileSystemMock, removeFilesInDirectory("/installroot/base/update/var/processedReports")).Times(1);

    EXPECT_CALL(fileSystemMock, writeFile("/installroot/base/update/var/processedReports/update_report_1.json", _)).Times(1);
    EXPECT_CALL(fileSystemMock, writeFile("/installroot/base/update/var/processedReports/update_report_2.json", _)).Times(1);
    EXPECT_CALL(fileSystemMock, copyFile("/installroot/base/update/var/update_config.json", _)).Times(1);


    EXPECT_CALL(*api, sendEvent("ALC", _)).Times(0);
    EXPECT_CALL(*api, sendStatus("ALC", _, _));

    std::future<void> schedulerRunHandle =
            std::async(std::launch::async, [&updateScheduler]() { updateScheduler.mainLoop(); });

    m_queue->push(SchedulerTask{ SchedulerTask::TaskType::Policy, updatePolicyWithProxy });

    m_queue->push(SchedulerTask{ SchedulerTask::TaskType::UpdateNow, updateAction });

    // on success... runner will add task to the queue
    m_queue->push(SchedulerTask{ SchedulerTask::TaskType::SulDownloaderFinished, "" });

    m_queue->push(SchedulerTask{ SchedulerTask::TaskType::ShutdownReceived, "" });
    schedulerRunHandle.get(); // synchronize stop

    EXPECT_EQ(scheduledUpdate.getEnabled(), false);
}








TEST_F(TestUpdateScheduler, checkUpdateOnStartUpNotSetToFalseWhenMissedUpdate) // NOLINT
{
    ICronSchedulerThread::DurationTime time = std::chrono::minutes(1);
    EXPECT_CALL(*cron, setPeriodTime(time));
    ICronSchedulerThread::DurationTime schedulerTime = std::chrono::minutes(40);
    EXPECT_CALL(*cron, setPeriodTime(schedulerTime));
    EXPECT_CALL(*cron, requestStop());

    ScheduledUpdate scheduledUpdate;
    EXPECT_CALL(*cron, setScheduledUpdate(_)).WillOnce(SaveArg<0>(&scheduledUpdate));

    EXPECT_CALL(*runner, isRunning()).WillOnce(Return(false));
    auto& fileSystemMock = setupFileSystemMock();

    EXPECT_CALL(*api, sendEvent("ALC", _));
    EXPECT_CALL(*api, sendStatus("ALC", _, _));

    UpdateSchedulerImpl::UpdateSchedulerProcessor updateScheduler(
        m_queue,
        std::unique_ptr<IBaseServiceApi>(api),
        m_pluginCallback,
        std::unique_ptr<ICronSchedulerThread>(cron),
        std::unique_ptr<IAsyncSulDownloaderRunner>(runner));

    std::string reportPath =
        Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderReportGeneratedFilePath();

    EXPECT_CALL(fileSystemMock, writeFile("/installroot/base/update/var/update_config.json", _));
    EXPECT_CALL(fileSystemMock, isFile("/installroot/base/update/var/update_config.json")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isFile(HasSubstr("/installroot/base/update/var/update_report")))
        .WillOnce(Return(true))   // after the first report is created
        .WillOnce(Return(false)); // after checking if it is safe to move file
    EXPECT_CALL(fileSystemMock, moveFile(reportPath, _));
    // the real name would be different as the file has been moved. But, ignoring here.
    std::vector<std::string> files{ reportPath, "/installroot/base/update/var/update_config.json" };
    std::vector<std::string> noReportFiles{ "/installroot/base/update/var/update_config.json" };
    EXPECT_CALL(fileSystemMock, listFiles("/installroot/base/update/var"))
        .Times(2)
        .WillRepeatedly(Return(files)); // after policy it process current reports (empty)

    EXPECT_CALL(fileSystemMock, isFile("/installroot/base/update/var/processedReports/update_report.json")).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, writeFile("/installroot/base/update/var/processedReports/update_report.json", _));

    // Set last update time to a week ago (in case the test runs at the same time as the scheduled update)
    time_t nowTime = std::time(nullptr) - 7 * 24 * 60 * 60;
    std::tm now = *std::localtime(&nowTime);
    char formattedTime[18];
    strftime(formattedTime, 18, "%Y%m%d %H:%M:%S", &now);
    std::string timeString(formattedTime);

    std::string newDownloadReport =
        Common::UtilityImpl::StringUtils::replaceAll(downloadReport, R"sophos(20180810 11:00:00)sophos", timeString);

    EXPECT_CALL(fileSystemMock, readFile(reportPath)).WillOnce(Return(newDownloadReport));

    std::future<void> schedulerRunHandle =
        std::async(std::launch::async, [&updateScheduler]() { updateScheduler.mainLoop(); });

    m_queue->push(SchedulerTask{ SchedulerTask::TaskType::Policy, updatePolicyWithScheduledUpdate });

    m_queue->push(SchedulerTask{ SchedulerTask::TaskType::ShutdownReceived, "" });
    schedulerRunHandle.get(); // synchronize stop

    EXPECT_EQ(scheduledUpdate.getEnabled(), true);
}

TEST_F(TestUpdateScheduler, checkUpdateOnStartUpSetToFalseWhenNotMissedUpdate) // NOLINT
{
    ICronSchedulerThread::DurationTime time = std::chrono::minutes(1);
    EXPECT_CALL(*cron, setPeriodTime(time));
    ICronSchedulerThread::DurationTime schedulerTime = std::chrono::minutes(40);
    EXPECT_CALL(*cron, setPeriodTime(schedulerTime));
    EXPECT_CALL(*cron, requestStop());

    ScheduledUpdate scheduledUpdate;
    EXPECT_CALL(*cron, setScheduledUpdate(_)).WillOnce(SaveArg<0>(&scheduledUpdate));
    EXPECT_CALL(*cron, setUpdateOnStartUp(false));

    EXPECT_CALL(*runner, isRunning()).WillOnce(Return(false));
    auto& fileSystemMock = setupFileSystemMock();

    EXPECT_CALL(*api, sendEvent("ALC", _));
    EXPECT_CALL(*api, sendStatus("ALC", _, _));

    UpdateSchedulerImpl::UpdateSchedulerProcessor updateScheduler(
        m_queue,
        std::unique_ptr<IBaseServiceApi>(api),
        m_pluginCallback,
        std::unique_ptr<ICronSchedulerThread>(cron),
        std::unique_ptr<IAsyncSulDownloaderRunner>(runner));

    std::string reportPath =
        Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderReportGeneratedFilePath();

    EXPECT_CALL(fileSystemMock, writeFile("/installroot/base/update/var/update_config.json", _));
    EXPECT_CALL(fileSystemMock, isFile("/installroot/base/update/var/update_config.json")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isFile(HasSubstr("/installroot/base/update/var/update_report")))
        .WillOnce(Return(true))   // after the first report is created
        .WillOnce(Return(false)); // after checking if it is safe to move file
    EXPECT_CALL(fileSystemMock, moveFile(reportPath, _));
    // the real name would be different as the file has been moved. But, ignoring here.
    std::vector<std::string> files{ reportPath, "/installroot/base/update/var/update_config.json" };
    std::vector<std::string> noReportFiles{ "/installroot/base/update/var/update_config.json" };
    EXPECT_CALL(fileSystemMock, listFiles("/installroot/base/update/var"))
        .Times(2)
        .WillRepeatedly(Return(files)); // after policy it process current reports (empty)

    EXPECT_CALL(fileSystemMock, isFile("/installroot/base/update/var/processedReports/update_report.json")).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, writeFile("/installroot/base/update/var/processedReports/update_report.json", _));

    time_t nowTime = std::time(nullptr);
    std::tm now = *std::localtime(&nowTime);
    char formattedTime[18];
    strftime(formattedTime, 18, "%Y%m%d %H:%M:%S", &now);
    std::string timeString(formattedTime);

    std::string newDownloadReport =
        Common::UtilityImpl::StringUtils::replaceAll(downloadReport, R"sophos(20180810 11:00:00)sophos", timeString);

    EXPECT_CALL(fileSystemMock, readFile(reportPath)).WillOnce(Return(newDownloadReport));

    std::future<void> schedulerRunHandle =
        std::async(std::launch::async, [&updateScheduler]() { updateScheduler.mainLoop(); });

    m_queue->push(SchedulerTask{ SchedulerTask::TaskType::Policy, updatePolicyWithScheduledUpdate });

    m_queue->push(SchedulerTask{ SchedulerTask::TaskType::ShutdownReceived, "" });
    schedulerRunHandle.get(); // synchronize stop

    EXPECT_EQ(scheduledUpdate.getEnabled(), true);
}

TEST_F(TestUpdateScheduler, invalidPolicyWillNotCreateConfig) // NOLINT
{
    ICronSchedulerThread::DurationTime time = std::chrono::minutes(50);
    EXPECT_CALL(*cron, setPeriodTime(time)).Times(0);
    EXPECT_CALL(*cron, setScheduledUpdate(_)).Times(0);

    EXPECT_CALL(*cron, requestStop());
    EXPECT_CALL(*runner, isRunning()).WillOnce(Return(false));

    auto& fileSystemMock = setupFileSystemMock();

    UpdateSchedulerImpl::UpdateSchedulerProcessor updateScheduler(
        m_queue,
        std::unique_ptr<IBaseServiceApi>(api),
        m_pluginCallback,
        std::unique_ptr<ICronSchedulerThread>(cron),
        std::unique_ptr<IAsyncSulDownloaderRunner>(runner));

    EXPECT_CALL(fileSystemMock, writeFile("/installroot/base/update/var/update_config.json", _)).Times(0);
    EXPECT_CALL(fileSystemMock, isFile("/installroot/base/update/var/update_config.json")).Times(0);
    std::future<void> schedulerRunHandle =
        std::async(std::launch::async, [&updateScheduler]() { updateScheduler.mainLoop(); });

    std::string invalidPolicyEmptyPassword = Common::UtilityImpl::StringUtils::orderedStringReplace(
        updatePolicyWithProxy, { { "UserPassword=\"54m5ung\"", "UserPassword=\"\"" } });

    m_queue->push(SchedulerTask{ SchedulerTask::TaskType::Policy, invalidPolicyEmptyPassword });
    std::string invalidPolicyEmptyUserName= Common::UtilityImpl::StringUtils::orderedStringReplace(
            updatePolicyWithProxy, { { "UserName=\"QA940267\"", "UserName=\"\"" } });

    m_queue->push(SchedulerTask{ SchedulerTask::TaskType::Policy, invalidPolicyEmptyUserName });
    m_queue->push(SchedulerTask{ SchedulerTask::TaskType::ShutdownReceived, "" });
    schedulerRunHandle.get(); // synchronize stop
}

TEST_F(TestUpdateScheduler, PolicyWithInvalidPolicyPeriodWillCreateConfig) // NOLINT
{
    ICronSchedulerThread::DurationTime time = std::chrono::minutes(50);
    EXPECT_CALL(*cron, setScheduledUpdate(_)).Times(1);

    EXPECT_CALL(*cron, requestStop());
    EXPECT_CALL(*runner, isRunning()).WillOnce(Return(false));

    auto& fileSystemMock = setupFileSystemMock();

    UpdateSchedulerImpl::UpdateSchedulerProcessor updateScheduler(
            m_queue,
            std::unique_ptr<IBaseServiceApi>(api),
            m_pluginCallback,
            std::unique_ptr<ICronSchedulerThread>(cron),
            std::unique_ptr<IAsyncSulDownloaderRunner>(runner));

    EXPECT_CALL(fileSystemMock, writeFile("/installroot/base/update/var/update_config.json", _)).Times(1);
    EXPECT_CALL(fileSystemMock, isFile("/installroot/base/update/var/update_config.json"))
            .Times(1)
            .WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, isFile("/installroot/base/update/var/update_report.json")).WillOnce(Return(false));

    std::future<void> schedulerRunHandle =
            std::async(std::launch::async, [&updateScheduler]() { updateScheduler.mainLoop(); });

    std::string invalidPolicyPeriod = Common::UtilityImpl::StringUtils::orderedStringReplace(
            updatePolicyWithProxy,
            { { R"sophos(SchedEnable="true" Frequency="50")sophos",
                      R"sophos(SchedEnable="true" Frequency="50000000")sophos" } });

    m_queue->push(SchedulerTask{ SchedulerTask::TaskType::Policy, invalidPolicyPeriod });

    m_queue->push(SchedulerTask{ SchedulerTask::TaskType::ShutdownReceived, "" });
    schedulerRunHandle.get(); // synchronize stop
}
TEST_F(TestUpdateScheduler, scheduledUpdatePolicyWillConfigureSchedule) // NOLINT
{
    ICronSchedulerThread::DurationTime time = std::chrono::minutes(1);
    EXPECT_CALL(*cron, setPeriodTime(time));
    ICronSchedulerThread::DurationTime schedulerTime = std::chrono::minutes(40);
    EXPECT_CALL(*cron, setPeriodTime(schedulerTime));
    EXPECT_CALL(*cron, requestStop());

    ScheduledUpdate scheduledUpdate;
    EXPECT_CALL(*cron, setScheduledUpdate(_)).WillOnce(SaveArg<0>(&scheduledUpdate));

    EXPECT_CALL(*runner, isRunning()).WillOnce(Return(false));
    auto& fileSystemMock = setupFileSystemMock();

    UpdateSchedulerImpl::UpdateSchedulerProcessor updateScheduler(
        m_queue,
        std::unique_ptr<IBaseServiceApi>(api),
        m_pluginCallback,
        std::unique_ptr<ICronSchedulerThread>(cron),
        std::unique_ptr<IAsyncSulDownloaderRunner>(runner));

    EXPECT_CALL(fileSystemMock, writeFile("/installroot/base/update/var/update_config.json", _));
    EXPECT_CALL(fileSystemMock, isFile("/installroot/base/update/var/update_config.json")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isFile("/installroot/base/update/var/update_report.json")).WillOnce(Return(false));

    std::future<void> schedulerRunHandle =
        std::async(std::launch::async, [&updateScheduler]() { updateScheduler.mainLoop(); });

    m_queue->push(SchedulerTask{ SchedulerTask::TaskType::Policy, updatePolicyWithScheduledUpdate });

    m_queue->push(SchedulerTask{ SchedulerTask::TaskType::ShutdownReceived, "" });
    schedulerRunHandle.get(); // synchronize stop

    // Saturday(6th day of the week), 9:41
    EXPECT_EQ(scheduledUpdate.getEnabled(), true);

    auto actualTime = scheduledUpdate.getScheduledTime();
    EXPECT_EQ(actualTime.hour, 9);
    EXPECT_EQ(actualTime.minute, 41);
    EXPECT_EQ(actualTime.weekDay, 6);
}

TEST_F(TestUpdateScheduler, badScheduledUpdatePolicyWillNotConfigureScheduleBadDay) // NOLINT
{
    ICronSchedulerThread::DurationTime time = std::chrono::minutes(40);
    EXPECT_CALL(*cron, setPeriodTime(time));
    EXPECT_CALL(*cron, requestStop());

    ScheduledUpdate scheduledUpdate;
    EXPECT_CALL(*cron, setScheduledUpdate(_)).WillOnce(SaveArg<0>(&scheduledUpdate));

    EXPECT_CALL(*runner, isRunning()).WillOnce(Return(false));
    auto& fileSystemMock = setupFileSystemMock();

    UpdateSchedulerImpl::UpdateSchedulerProcessor updateScheduler(
        m_queue,
        std::unique_ptr<IBaseServiceApi>(api),
        m_pluginCallback,
        std::unique_ptr<ICronSchedulerThread>(cron),
        std::unique_ptr<IAsyncSulDownloaderRunner>(runner));

    EXPECT_CALL(fileSystemMock, writeFile("/installroot/base/update/var/update_config.json", _));
    EXPECT_CALL(fileSystemMock, isFile("/installroot/base/update/var/update_config.json")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isFile("/installroot/base/update/var/update_report.json")).WillOnce(Return(false));

    std::future<void> schedulerRunHandle =
        std::async(std::launch::async, [&updateScheduler]() { updateScheduler.mainLoop(); });

    std::string invalidPolicyScheduleDay = Common::UtilityImpl::StringUtils::orderedStringReplace(
        updatePolicyWithScheduledUpdate,
        { { R"sophos(Day="Saturday" Time="09:41:00")sophos", R"sophos(Day="Blahday" Time="09:41:00")sophos" } });

    m_queue->push(SchedulerTask{ SchedulerTask::TaskType::Policy, invalidPolicyScheduleDay });

    m_queue->push(SchedulerTask{ SchedulerTask::TaskType::ShutdownReceived, "" });
    schedulerRunHandle.get(); // synchronize stop

    EXPECT_EQ(scheduledUpdate.getEnabled(), false);
}

TEST_F(TestUpdateScheduler, badScheduledUpdatePolicyWillNotConfigureScheduleBadTime) // NOLINT
{
    ICronSchedulerThread::DurationTime time = std::chrono::minutes(40);
    EXPECT_CALL(*cron, setPeriodTime(time));
    EXPECT_CALL(*cron, requestStop());

    ScheduledUpdate scheduledUpdate;
    EXPECT_CALL(*cron, setScheduledUpdate(_)).WillOnce(SaveArg<0>(&scheduledUpdate));

    EXPECT_CALL(*runner, isRunning()).WillOnce(Return(false));
    auto& fileSystemMock = setupFileSystemMock();

    UpdateSchedulerImpl::UpdateSchedulerProcessor updateScheduler(
        m_queue,
        std::unique_ptr<IBaseServiceApi>(api),
        m_pluginCallback,
        std::unique_ptr<ICronSchedulerThread>(cron),
        std::unique_ptr<IAsyncSulDownloaderRunner>(runner));

    EXPECT_CALL(fileSystemMock, writeFile("/installroot/base/update/var/update_config.json", _));
    EXPECT_CALL(fileSystemMock, isFile("/installroot/base/update/var/update_config.json")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isFile("/installroot/base/update/var/update_report.json")).WillOnce(Return(false));

    std::future<void> schedulerRunHandle =
        std::async(std::launch::async, [&updateScheduler]() { updateScheduler.mainLoop(); });

    std::string invalidPolicyScheduleTime = Common::UtilityImpl::StringUtils::orderedStringReplace(
        updatePolicyWithScheduledUpdate,
        { { R"sophos(Day="Saturday" Time="09:41:00")sophos", R"sophos(Day="Saturday" Time="24:00:00")sophos" } });

    m_queue->push(SchedulerTask{ SchedulerTask::TaskType::Policy, invalidPolicyScheduleTime });

    m_queue->push(SchedulerTask{ SchedulerTask::TaskType::ShutdownReceived, "" });
    schedulerRunHandle.get(); // synchronize stop

    EXPECT_EQ(scheduledUpdate.getEnabled(), false);
}

TEST_F(TestUpdateScheduler, badScheduledUpdatePolicyWillNotConfigureScheduleBadTimeType) // NOLINT
{
    ICronSchedulerThread::DurationTime time = std::chrono::minutes(40);
    EXPECT_CALL(*cron, setPeriodTime(time));
    EXPECT_CALL(*cron, requestStop());

    ScheduledUpdate scheduledUpdate;
    EXPECT_CALL(*cron, setScheduledUpdate(_)).WillOnce(SaveArg<0>(&scheduledUpdate));

    EXPECT_CALL(*runner, isRunning()).WillOnce(Return(false));
    auto& fileSystemMock = setupFileSystemMock();

    UpdateSchedulerImpl::UpdateSchedulerProcessor updateScheduler(
        m_queue,
        std::unique_ptr<IBaseServiceApi>(api),
        m_pluginCallback,
        std::unique_ptr<ICronSchedulerThread>(cron),
        std::unique_ptr<IAsyncSulDownloaderRunner>(runner));

    EXPECT_CALL(fileSystemMock, writeFile("/installroot/base/update/var/update_config.json", _));
    EXPECT_CALL(fileSystemMock, isFile("/installroot/base/update/var/update_config.json")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isFile("/installroot/base/update/var/update_report.json")).WillOnce(Return(false));

    std::future<void> schedulerRunHandle =
        std::async(std::launch::async, [&updateScheduler]() { updateScheduler.mainLoop(); });

    std::string invalidPolicyScheduleTimeType = Common::UtilityImpl::StringUtils::orderedStringReplace(
        updatePolicyWithScheduledUpdate,
        { { R"sophos(Day="Saturday" Time="09:41:00")sophos", R"sophos(Day="Monday" Time="a:a:a")sophos" } });

    m_queue->push(SchedulerTask{ SchedulerTask::TaskType::Policy, invalidPolicyScheduleTimeType });

    m_queue->push(SchedulerTask{ SchedulerTask::TaskType::ShutdownReceived, "" });
    schedulerRunHandle.get(); // synchronize stop

    EXPECT_EQ(scheduledUpdate.getEnabled(), false);
}

TEST_F(TestUpdateScheduler, badScheduledUpdatePolicyWillNotConfigureScheduleDayType) // NOLINT
{
    ICronSchedulerThread::DurationTime time = std::chrono::minutes(40);
    EXPECT_CALL(*cron, setPeriodTime(time));
    EXPECT_CALL(*cron, requestStop());

    ScheduledUpdate scheduledUpdate;
    EXPECT_CALL(*cron, setScheduledUpdate(_)).WillOnce(SaveArg<0>(&scheduledUpdate));

    EXPECT_CALL(*runner, isRunning()).WillOnce(Return(false));
    auto& fileSystemMock = setupFileSystemMock();

    UpdateSchedulerImpl::UpdateSchedulerProcessor updateScheduler(
        m_queue,
        std::unique_ptr<IBaseServiceApi>(api),
        m_pluginCallback,
        std::unique_ptr<ICronSchedulerThread>(cron),
        std::unique_ptr<IAsyncSulDownloaderRunner>(runner));

    EXPECT_CALL(fileSystemMock, writeFile("/installroot/base/update/var/update_config.json", _));
    EXPECT_CALL(fileSystemMock, isFile("/installroot/base/update/var/update_config.json")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isFile("/installroot/base/update/var/update_report.json")).WillOnce(Return(false));

    std::future<void> schedulerRunHandle =
        std::async(std::launch::async, [&updateScheduler]() { updateScheduler.mainLoop(); });

    std::string invalidPolicyScheduleDayType = Common::UtilityImpl::StringUtils::orderedStringReplace(
        updatePolicyWithScheduledUpdate,
        { { R"sophos(Day="Saturday" Time="09:41:00")sophos", R"sophos(Day="0" Time="11:11:11")sophos" } });

    m_queue->push(SchedulerTask{ SchedulerTask::TaskType::Policy, invalidPolicyScheduleDayType });

    m_queue->push(SchedulerTask{ SchedulerTask::TaskType::ShutdownReceived, "" });
    schedulerRunHandle.get(); // synchronize stop

    EXPECT_EQ(scheduledUpdate.getEnabled(), false);
}

TEST_F(TestUpdateScheduler, policyAfterInstallConfiguresSulDownloaderAndTriggersUpdate) // NOLINT
{
    ICronSchedulerThread::DurationTime time = std::chrono::minutes(50);
    EXPECT_CALL(*cron, setPeriodTime(time));
    EXPECT_CALL(*cron, requestStop());

    ScheduledUpdate scheduledUpdate;
    EXPECT_CALL(*cron, setScheduledUpdate(_)).WillOnce(SaveArg<0>(&scheduledUpdate));

    EXPECT_CALL(*runner, isRunning()).Times(2).WillRepeatedly(Return(false));
    auto& fileSystemMock = setupFileSystemMock();

    UpdateSchedulerImpl::UpdateSchedulerProcessor updateScheduler(
        m_queue,
        std::unique_ptr<IBaseServiceApi>(api),
        m_pluginCallback,
        std::unique_ptr<ICronSchedulerThread>(cron),
        std::unique_ptr<IAsyncSulDownloaderRunner>(runner));

    EXPECT_CALL(fileSystemMock, writeFile("/installroot/base/update/var/update_config.json", _));
    EXPECT_CALL(fileSystemMock, isFile("/installroot/base/update/var/update_report.json")).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, isFile("/installroot/base/update/var/update_config.json")).WillOnce(Return(true));
    EXPECT_CALL(fileSystemMock, isFile("/installroot/base/update/var/update_config.json"))
        .WillOnce(Return(false))
        .RetiresOnSaturation();

    EXPECT_CALL(*runner, triggerSulDownloader());

    std::future<void> schedulerRunHandle =
        std::async(std::launch::async, [&updateScheduler]() { updateScheduler.mainLoop(); });

    m_queue->push(SchedulerTask{ SchedulerTask::TaskType::Policy, updatePolicyWithProxy });

    m_queue->push(SchedulerTask{ SchedulerTask::TaskType::ShutdownReceived, "" });
    schedulerRunHandle.get(); // synchronize stop

    EXPECT_EQ(scheduledUpdate.getEnabled(), false);
}

TEST_F(TestUpdateScheduler, policyChangeDoesNotTriggerSulDownloaderToUpdate) // NOLINT
{
    ICronSchedulerThread::DurationTime time = std::chrono::minutes(50);
    EXPECT_CALL(*cron, setPeriodTime(time));
    EXPECT_CALL(*cron, requestStop());

    ScheduledUpdate scheduledUpdate;
    EXPECT_CALL(*cron, setScheduledUpdate(_)).WillOnce(SaveArg<0>(&scheduledUpdate));

    EXPECT_CALL(*runner, isRunning()).WillOnce(Return(false));
    auto& fileSystemMock = setupFileSystemMock();

    UpdateSchedulerImpl::UpdateSchedulerProcessor updateScheduler(
        m_queue,
        std::unique_ptr<IBaseServiceApi>(api),
        m_pluginCallback,
        std::unique_ptr<ICronSchedulerThread>(cron),
        std::unique_ptr<IAsyncSulDownloaderRunner>(runner));

    EXPECT_CALL(fileSystemMock, writeFile("/installroot/base/update/var/update_config.json", _));
    EXPECT_CALL(fileSystemMock, isFile("/installroot/base/update/var/update_report.json")).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, isFile("/installroot/base/update/var/update_config.json")).WillOnce(Return(true));

    EXPECT_CALL(*runner, triggerSulDownloader()).Times(0);

    std::future<void> schedulerRunHandle =
        std::async(std::launch::async, [&updateScheduler]() { updateScheduler.mainLoop(); });

    m_queue->push(SchedulerTask{ SchedulerTask::TaskType::Policy, updatePolicyWithProxy });

    m_queue->push(SchedulerTask{ SchedulerTask::TaskType::ShutdownReceived, "" });
    schedulerRunHandle.get(); // synchronize stop

    EXPECT_EQ(scheduledUpdate.getEnabled(), false);
}

TEST_F(TestUpdateScheduler, sulDownloaderTimeoutWillTriggerSulDownloaderToUpdate) // NOLINT
{
    ICronSchedulerThread::DurationTime time = std::chrono::minutes(50);
    EXPECT_CALL(*cron, setPeriodTime(time));
    // update now restart the chron time
    EXPECT_CALL(*cron, reset());
    EXPECT_CALL(*cron, requestStop());

    ScheduledUpdate scheduledUpdate;
    EXPECT_CALL(*cron, setScheduledUpdate(_)).WillOnce(SaveArg<0>(&scheduledUpdate));

    EXPECT_CALL(*runner, isRunning()).WillOnce(Return(false));
    EXPECT_CALL(*runner, isRunning()).WillOnce(Return(true)).RetiresOnSaturation();
    EXPECT_CALL(*runner, hasTimedOut()).WillOnce(Return(true));
    EXPECT_CALL(*runner, triggerSulDownloader());
    auto& fileSystemMock = setupFileSystemMock();

    UpdateSchedulerImpl::UpdateSchedulerProcessor updateScheduler(
            m_queue,
            std::unique_ptr<IBaseServiceApi>(api),
            m_pluginCallback,
            std::unique_ptr<ICronSchedulerThread>(cron),
            std::unique_ptr<IAsyncSulDownloaderRunner>(runner));

    std::string reportPath =
            Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderReportGeneratedFilePath();

    EXPECT_CALL(fileSystemMock, writeFile("/installroot/base/update/var/update_config.json", _));
    EXPECT_CALL(fileSystemMock, isFile("/installroot/base/update/var/update_config.json"))
            .Times(2)
            .WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, isFile(HasSubstr("/installroot/base/update/var/update_report")))
            .WillOnce(Return(false))  // after policy no report exist
            .WillOnce(Return(true))   // after the first report is created
            .WillOnce(Return(false)); // after checking if it is safe to move file
    EXPECT_CALL(fileSystemMock, moveFile(reportPath, _));
    // the real name would be different as the file has been moved. But, ignoring here.
    std::vector<std::string> files{ reportPath, "/installroot/base/update/var/update_config.json" };
    std::vector<std::string> noReportFiles{ "/installroot/base/update/var/update_config.json" };
    EXPECT_CALL(fileSystemMock, listFiles("/installroot/base/update/var"))
            .WillOnce(Return(files)); // report generated by suldownloader

    EXPECT_CALL(fileSystemMock, isFile("/installroot/base/update/var/processedReports/update_report.json")).WillOnce(Return(false));
    EXPECT_CALL(fileSystemMock, writeFile("/installroot/base/update/var/processedReports/update_report.json", _));

    EXPECT_CALL(fileSystemMock, listFiles("/installroot/base/update/var"))
            .Times(2)
            .WillRepeatedly(Return(noReportFiles)) // after policy it process current reports (empty)
            .RetiresOnSaturation();

    EXPECT_CALL(fileSystemMock, readFile(reportPath)).WillOnce(Return(downloadReport));

    EXPECT_CALL(*api, sendEvent("ALC", _));
    EXPECT_CALL(*api, sendStatus("ALC", _, _));

    std::future<void> schedulerRunHandle =
            std::async(std::launch::async, [&updateScheduler]() { updateScheduler.mainLoop(); });

    m_queue->push(SchedulerTask{ SchedulerTask::TaskType::Policy, updatePolicyWithProxy });

    m_queue->push(SchedulerTask{ SchedulerTask::TaskType::UpdateNow, updateAction });

    // on success... runner will add task to the queue
    m_queue->push(SchedulerTask{ SchedulerTask::TaskType::SulDownloaderFinished, "" });

    m_queue->push(SchedulerTask{ SchedulerTask::TaskType::ShutdownReceived, "" });
    schedulerRunHandle.get(); // synchronize stop

    EXPECT_EQ(scheduledUpdate.getEnabled(), false);
}

TEST_F(TestUpdateScheduler, sulDownloaderNotTimeoutWillNotTriggerSulDownloaderToUpdate) // NOLINT
{
    ICronSchedulerThread::DurationTime time = std::chrono::minutes(50);
    EXPECT_CALL(*cron, setPeriodTime(time));
    // update now restart the chron time
    EXPECT_CALL(*cron, reset());
    EXPECT_CALL(*cron, requestStop());

    ScheduledUpdate scheduledUpdate;
    EXPECT_CALL(*cron, setScheduledUpdate(_)).WillOnce(SaveArg<0>(&scheduledUpdate));

    EXPECT_CALL(*runner, isRunning()).WillOnce(Return(false));
    EXPECT_CALL(*runner, isRunning()).WillOnce(Return(true)).RetiresOnSaturation();
    EXPECT_CALL(*runner, hasTimedOut()).WillOnce(Return(false));

    // Ensure Sul Downloader is not triggered if sul downloader has not timed out.
    EXPECT_CALL(*runner, triggerSulDownloader()).Times(0);
    auto& fileSystemMock = setupFileSystemMock();

    UpdateSchedulerImpl::UpdateSchedulerProcessor updateScheduler(
            m_queue,
            std::unique_ptr<IBaseServiceApi>(api),
            m_pluginCallback,
            std::unique_ptr<ICronSchedulerThread>(cron),
            std::unique_ptr<IAsyncSulDownloaderRunner>(runner));

    std::string reportPath =
            Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderReportGeneratedFilePath();

    EXPECT_CALL(fileSystemMock, writeFile("/installroot/base/update/var/update_config.json", _));
    EXPECT_CALL(fileSystemMock, isFile("/installroot/base/update/var/update_config.json"))
            .Times(1)
            .WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystemMock, isFile(HasSubstr("/installroot/base/update/var/update_report")))
            .WillOnce(Return(false));  // after policy no report exist

    // the real name would be different as the file has been moved. But, ignoring here.
    std::vector<std::string> files{ reportPath, "/installroot/base/update/var/update_config.json" };
    std::vector<std::string> noReportFiles{ "/installroot/base/update/var/update_config.json" };

    EXPECT_CALL(fileSystemMock, listFiles("/installroot/base/update/var"))
            .Times(2)
            .WillRepeatedly(Return(noReportFiles)) // after policy it process current reports (empty)
            .RetiresOnSaturation();

    std::future<void> schedulerRunHandle =
            std::async(std::launch::async, [&updateScheduler]() { updateScheduler.mainLoop(); });

    m_queue->push(SchedulerTask{ SchedulerTask::TaskType::Policy, updatePolicyWithProxy });

    m_queue->push(SchedulerTask{ SchedulerTask::TaskType::UpdateNow, updateAction });

    m_queue->push(SchedulerTask{ SchedulerTask::TaskType::ShutdownReceived, "" });
    schedulerRunHandle.get(); // synchronize stop

    EXPECT_EQ(scheduledUpdate.getEnabled(), false);
}






