// Copyright 2023 Sophos Limited. All rights reserved.

#include "modules/Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "modules/Common/Policy/ALCPolicy.h"
#include "modules/Common/Policy/PolicyParseException.h"

#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/MemoryAppender.h"
#include "tests/Common/Helpers/MockFileSystem.h"

#include <gtest/gtest.h>

// NOLINTNEXTLINE(cert-err58-cpp)
static const std::string updatePolicyWithCache{ R"sophos(<?xml version="1.0"?>
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
      <subscription Id="Base" RigidName="ServerProtectionLinux-Base" Tag="RECOMMENDED" BaseVersion="10" FixedVersion="11"/>
      <subscription Id="Base" RigidName="ServerProtectionLinux-Base9" Tag="RECOMMENDED" BaseVersion="9" FixedVersion="8"/>
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

// NOLINTNEXTLINE(cert-err58-cpp)
static const std::string updatePolicyWithProxy{ R"sophos(<?xml version="1.0"?>
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
      <subscription Id="Base" RigidName="ServerProtectionLinux-Base9" Tag="RECOMMENDED" BaseVersion="9"/>
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

// NOLINTNEXTLINE(cert-err58-cpp)
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
      <subscription Id="Base" RigidName="ServerProtectionLinux-Base9" Tag="RECOMMENDED" BaseVersion="9"/>
    </cloud_subscriptions>
    <delay_supplements enabled="true"/>
    <delay_updating Day="Wednesday" Time="17:05:00"/>
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

// NOLINTNEXTLINE(cert-err58-cpp)
static const std::string updatePolicyWithAESCredential{ R"sophos(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
  <csc:Comp RevID="f6babe12a13a5b2134c5861d01aed0eaddc20ea374e3a717ee1ea1451f5e2cf6" policyType="1"/>
  <AUConfig platform="Linux">
    <sophos_address address="http://es-web.sophos.com/update"/>
    <primary_location>
      <server BandwidthLimit="256" AutoDial="false" Algorithm="AES256" UserPassword="CCAcWWDAL1sCAV1YiHE20dTJIXMaTLuxrBppRLRbXgGOmQBrysz16sn7RuzXPaX6XHk=" UserName="QA940267" UseSophos="true" UseHttps="false" UseDelta="true" ConnectionAddress="" AllowLocalConfig="false"/>
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
      <subscription Id="Base" RigidName="ServerProtectionLinux-Base9" Tag="RECOMMENDED" BaseVersion="9"/>
    </cloud_subscriptions>
    <delay_supplements enabled="true"/>
    <delay_updating Day="Wednesday" Time="17:00:00"/>
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

// NOLINTNEXTLINE(cert-err58-cpp)
static const std::string incorrectPolicyTypeXml{ R"sophos(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
  <csc:Comp RevID="f6babe12a13a5b2134c5861d01aed0eaddc20ea374e3a717ee1ea1451f5e2cf6" policyType="2"/>
</AUConfigurations>
)sophos" };

// NOLINTNEXTLINE(cert-err58-cpp)
static const std::string cacheCertificates{ R"sophos(-----BEGIN CERTIFICATE-----
MIIDxDCCAqygAwIBAgIQEFgFEJ0SYXZx+dHI2UTIVzANBgkqhkiG9w0BAQsFADBh
MQswCQYDVQQGEwJHQjEPMA0GA1UEBwwGT3hmb3JkMQ8wDQYDVQQKDAZTb3Bob3Mx
EjAQBgNVBAsMCUhlYXJ0YmVhdDEcMBoGA1UEAwwTSGVhcnRiZWF0LWV1LXdlc3Qt
MTAeFw0xNzExMDkwNzE1MDRaFw0xOTExMTMwNzE1MDRaMIGyMQswCQYDVQQGEwJH
QjEPMA0GA1UEBwwGT3hmb3JkMQ8wDQYDVQQKDAZTb3Bob3MxEjAQBgNVBAsMCUhl
YXJ0YmVhdDEYMBYGA1UEAwwPUHJvZHVjdCBMaWNlbnNlMTgwNgYDVQQDDC9DdXN0
b21lcklEOjRiNGNhM2JhLWMxNDQtNDQ0Ny04MDUwLTZjOTZhNzEwNGMxMTEZMBcG
A1UEAwwQU29waG9zIEhlYXJ0YmVhdDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCC
AQoCggEBAJQSXwmoOsdPRBQUI8f8i7Z6+UhNwvKPoM/kyMbwf2DG8FYPrpiG6uhR
JT7SBRGcWZstWIsafrj+RrXrXjapm0xGqSNc1nIuxQDcXJmmq74XCGedHiQGLF+f
BEDvdYd2KHrYwrg0nGuJB8tPz2s2MRcQLnx30eBL5KAMOkMoZMbkNnCHI8FycTDS
jwjqjN9u3JOMvbA1YEVDonDe39v0Oxj1sY93Oh3xG44496vhhy8r6nwdY9L9DCwn
OU/1wzGapNgDl7ox1JIFIlq+z5iaeGLVp2peVDtPsK8pJBISr6NeG0Mm6wMKkMre
w/Nm8Nu9bV+ocbG/xYncbjP8BYwhkYMCAwEAAaMmMCQwEgYDVR0TAQH/BAgwBgEB
/wIBADAOBgNVHQ8BAf8EBAMCAQYwDQYJKoZIhvcNAQELBQADggEBAFXtbg/c7t1t
MAZFFKUgtkEB5Hh2OBKPS5F87BPfZQD1RLUcbnB+mIuqpAIqHE/3ovtV7KBe0wFz
WgcN08w0Q4yAJV5eNsBZiqZYYRweB157vo/6oY1wvqB6D0tTcfSDG8/kthLITecC
C9jtK5rIwF4pAA4WKkAY6VT+uDCZU+Hejsutc7Ey9Tpon2fCpcmd6HizzCNWeRnG
6YtOsPIjQMToz7b3SnpE2m1zk4jmchQX55yn0rEAf2f/XqblPaEPpoySSNhx/uTg
zhGZIvpcyR9tIr43hyx2iR+E1TH7DqGeeHcbr8pQyd6UAGK9TAA1oDDuejXpQ/GY
c2uueUDEuWQ=
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIIDnTCCAoWgAwIBAgIBAjANBgkqhkiG9w0BAQsFADB/MQswCQYDVQQGEwJHQjEP
MA0GA1UEBwwGT3hmb3JkMQ8wDQYDVQQKDAZTb3Bob3MxFDASBgNVBAsMC0VuZ2lu
ZWVyaW5nMTgwNgYDVQQDDC9Tb3Bob3MgSGVhcnRiZWF0IE5vdC1Gb3ItUmVsZWFz
ZSBSb290IEF1dGhvcml0eTAeFw0xNTA2MDQyMTQyMTNaFw0zNTA2MDQyMTQyMTNa
MGExCzAJBgNVBAYTAkdCMQ8wDQYDVQQHDAZPeGZvcmQxDzANBgNVBAoMBlNvcGhv
czESMBAGA1UECwwJSGVhcnRiZWF0MRwwGgYDVQQDDBNIZWFydGJlYXQtZXUtd2Vz
dC0xMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAlJSxtUjPwyFrldV4
5TJlhP0Kmko3NyHyRqI9xsMRzJ6ZYgJWx1gpPkoT0L4EU+ZStdKbuizL5lhZfBQj
FmxMBLss+tD3WUIE1LQY2Ge+XzbhpTv2QPEslf/nX6T7uEKf3Sb7nsq6JbV3wOjV
R4rsehbUT5tPb0QZAXBu3Cej5ppQrEeQMLeVdU9js8pAKK1HhHJPXcQEjw2SPRGa
S8UnXkK5nd+myQYAgrCtplD0Ul35U2Kc0lRsyVeQsupHlNN5we7HfSI4TrPHox7B
rjAEmQ77c9uP6PbXqY8buo8TlT33eoUYJwFd9ivXutFRf0l8QO2u6Jg+W7eCQqp9
uNYHYwIDAQABo0IwQDAdBgNVHQ4EFgQUrv/9VM4ZJ2TRU4LJ6n3u0YPY8bEwDwYD
VR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMCAQYwDQYJKoZIhvcNAQELBQADggEB
ALIoFNI+pvj57lL83AS1rZX2V6CvGjCiFpR0YfJXlKCMFX9QOm1zT8OKkaLdMAsm
BQZuwhL2k3byk5b8gHYn9IcVE55RC5F2WUsdwke27BIalSSPNitc5sIxKBoc4BLS
2pkhIDwqjTqX9HM31IVT/GlnYqubh1kigqgdOQOIRfCKpb17TZoRQHbC7N5gOoa7
xNwKx/Z1lc6IsUFSFuevVRIhlY0FL0xBIKveMYIBJFgZGY+hRCevqIu1sR0OYNHg
nJwgZ6PRCYPMsEntjoFEy5IoVheP/1yHCC/1seH9nhtGe7xzjuUJHwfPm9kBOpCU
zJqTw7msuYhp6kd4xCw7ccc=
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIIDuzCCAqOgAwIBAgIBATANBgkqhkiG9w0BAQsFADB/MQswCQYDVQQGEwJHQjEP
MA0GA1UEBwwGT3hmb3JkMQ8wDQYDVQQKDAZTb3Bob3MxFDASBgNVBAsMC0VuZ2lu
ZWVyaW5nMTgwNgYDVQQDDC9Tb3Bob3MgSGVhcnRiZWF0IE5vdC1Gb3ItUmVsZWFz
ZSBSb290IEF1dGhvcml0eTAeFw0xNTA2MDQyMTQyMDJaFw0zNTA2MDQyMTQyMDJa
MH8xCzAJBgNVBAYTAkdCMQ8wDQYDVQQHDAZPeGZvcmQxDzANBgNVBAoMBlNvcGhv
czEUMBIGA1UECwwLRW5naW5lZXJpbmcxODA2BgNVBAMML1NvcGhvcyBIZWFydGJl
YXQgTm90LUZvci1SZWxlYXNlIFJvb3QgQXV0aG9yaXR5MIIBIjANBgkqhkiG9w0B
AQEFAAOCAQ8AMIIBCgKCAQEAtQqlMqhJNYlTp/4zElmB7WOEsFUCLVD+Kzneh733
JbpL+RgRQ9qn76+J9vcFPG+eA5zf8M8Mbmb7IBEuLMmD5erT00AwoH8Vr4UP9Wb4
wPn8QzI+WO5oO3NjGvVGD/cufjBH9zqi8oiQwYELrz8fHXTxkCAJHcLp4tM86SQc
QnkWQv9TEWbGgrhJnMg3DAY8pERdLBhjVHPK15ZwyvMlqISBVOm9k8c81fwx4pfG
b4O+1CylPUKqreUXw7T6HB85+ym3TAkq9eeiJA1BkjdhWdpewIII5U955eaWJXaA
ra1jdNb21JAbrwy67Vp4v3uQ+62pOkubThlaQ+qIajJsHwIDAQABo0IwQDAdBgNV
HQ4EFgQUI6jNsBYvHqSsWqJfTrCMizzCHx0wDwYDVR0TAQH/BAUwAwEB/zAOBgNV
HQ8BAf8EBAMCAQYwDQYJKoZIhvcNAQELBQADggEBAE4EtR6dARf1ZqnfQlbjKubN
GGrDVv3GOmPenAH4AcCrXNe6ObIadJFVKWRYMvvMdb4GxxEts29OZdHnFK+Vvrqh
yUV0o4It14BBNjLIApo9eddShyDEZKGusQ33S9XYCIG6Yan59jOo2rvsntwi829v
B5yoGhSoxOaXlExk9YAGXaGmoaJ228iF2vuwmgSHJVxUy02AwiqqEwUy/MGL8877
wFkMtR8hrPVLP0hcHuzWN2cBmrl0C6TeKufqbZBqb/MPn2LWzKcvF44xs3k7uP/H
JWfkv6Tu5jsYGNkN3BSW0x/qjwz7XCSk2ZZxbCgZSq6LpB31sqZctnUxrYSpcdc=
-----END CERTIFICATE-----)sophos" };

// NOLINTNEXTLINE(cert-err58-cpp)
static const std::string mdrSSPLBasePolicy{ R"sophos(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
  <csc:Comp RevID="6d95c7ad1c25b034c94bea54fad38711e3f5057157c3468a8aafe3885f213802" policyType="1"/>
  <AUConfig platform="Linux">
    <sophos_address address="http://es-web.sophos.com/update"/>
    <primary_location>
      <server BandwidthLimit="256" AutoDial="false" Algorithm="Clear" UserPassword="password" UserName="CSP190408113225" UseSophos="true" UseHttps="true" UseDelta="true" ConnectionAddress="" AllowLocalConfig="false"/>
      <proxy ProxyType="0" ProxyUserPassword="" ProxyUserName="" ProxyPortNumber="0" ProxyAddress="" AllowLocalConfig="false"/>
    </primary_location>
    <secondary_location>
      <server BandwidthLimit="256" AutoDial="false" Algorithm="" UserPassword="" UserName="" UseSophos="false" UseHttps="true" UseDelta="true" ConnectionAddress="" AllowLocalConfig="false"/>
      <proxy ProxyType="0" ProxyUserPassword="" ProxyUserName="" ProxyPortNumber="0" ProxyAddress="" AllowLocalConfig="false"/>
    </secondary_location>
    <schedule AllowLocalConfig="false" SchedEnable="true" Frequency="60" DetectDialUp="false"/>
    <logging AllowLocalConfig="false" LogLevel="50" LogEnable="true" MaxLogFileSize="1"/>
    <bootstrap Location="" UsePrimaryServerAddress="true"/>
    <cloud_subscription RigidName="ServerProtectionLinux-Base" Tag="RECOMMENDED"/>
    <cloud_subscriptions>
      <subscription Id="Base" RigidName="ServerProtectionLinux-Base" Tag="RECOMMENDED"/>
      <subscription Id="MDR" RigidName="ServerProtectionLinux-Plugin-MDR" Tag="RECOMMENDED"/>
    </cloud_subscriptions>
    <delay_updating Day="Wednesday" Time="11:00:00"/>
    <delay_supplements enabled="true"/>
  </AUConfig>
  <Features>
    <Feature id="CORE"/>
    <Feature id="SDU"/>
    <Feature id="MDR"/>
  </Features>
  <intelligent_updating Enabled="false" SubscriptionPolicy="2DD71664-8D18-42C5-B3A0-FF0D289265BF"/>
  <customer id="8dd8c9f3-a9a1-84e2-49d8-f9320a76298e"/>
</AUConfigurations>
)sophos" };

namespace
{
    class TestALCPolicy : public MemoryAppenderUsingTests
    {
    public:
        TestALCPolicy() : MemoryAppenderUsingTests("Policy")
        {}
    };
}

//General Policy Tests

TEST_F(TestALCPolicy, constructWithEmptyString)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    EXPECT_THROW(Common::Policy::ALCPolicy empty{""}, Common::Policy::PolicyParseException);
    EXPECT_TRUE(appenderContains("Failed to parse policy: Error parsing xml: no element found"));
}

using namespace Common::Policy;

TEST_F(TestALCPolicy, invalidXml_unclosed_token)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    EXPECT_THROW(Common::Policy::ALCPolicy p{"<xml"}, Common::Policy::PolicyParseException);
    EXPECT_TRUE(appenderContains("Failed to parse policy: Error parsing xml: unclosed token"));
}

TEST_F(TestALCPolicy, invalidXml_mismatched_tag)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    EXPECT_THROW(Common::Policy::ALCPolicy p{"<xml><foo></xml>"}, Common::Policy::PolicyParseException);
    EXPECT_TRUE(appenderContains("Failed to parse policy: Error parsing xml: mismatched tag"));
}

TEST_F(TestALCPolicy, incorrectPolicyType)
{
    try
    {
        ALCPolicy obj{incorrectPolicyTypeXml};
        FAIL() << "Didn't fail due to incorrect policy type";
    }
    catch (const PolicyParseException& ex)
    {
        EXPECT_STREQ(ex.what(), "Update Policy type incorrect");
    }
}

TEST_F(TestALCPolicy, emptyPolicy)
{
    constexpr char emptyPolicy[] = R"sophos(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
  <csc:Comp RevID="b6a8fe2c0ce016c949016a5da2b7a089699271290ef7205d5bea0986768485d9" policyType="1"/>
</AUConfigurations>
)sophos";

    try
    {
        ALCPolicy obj{emptyPolicy};
        FAIL() << "Managed to parse empty policy";
    }
    catch (const PolicyParseException& ex)
    {
        EXPECT_STREQ(ex.what(), "Invalid policy: Username is empty");
    }
}

TEST_F(TestALCPolicy, minimumValidPolicy)
{
    constexpr char minPolicy[] = R"sophos(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
  <csc:Comp RevID="b6a8fe2c0ce016c949016a5da2b7a089699271290ef7205d5bea0986768485d9" policyType="1"/>
<AUConfig platform="Linux">
<primary_location>
  <server Algorithm="Clear" UserPassword="xxxxxx" UserName="W2YJXI6FED"/>
</primary_location>
</AUConfig>
</AUConfigurations>
)sophos";

    ALCPolicy obj{ minPolicy };
    auto settings = obj.getUpdateSettings();
    auto updatePeriod = obj.getUpdatePeriod();
    EXPECT_EQ(updatePeriod, std::chrono::minutes{60});
    EXPECT_EQ(obj.getSddsID(), "W2YJXI6FED");
    EXPECT_EQ(obj.getTelemetryHost(), "");
}

//sdds2 Update Server Tests

TEST_F(TestALCPolicy, sdds2_update_server)
{
    constexpr char minPolicy[] = R"sophos(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
  <csc:Comp RevID="b6a8fe2c0ce016c949016a5da2b7a089699271290ef7205d5bea0986768485d9" policyType="1"/>
<AUConfig platform="Linux">
<primary_location>
  <server Algorithm="Clear" UserPassword="xxxxxx" UserName="W2YJXI6FED" ConnectionAddress="http://dci.sophosupd.com/cloudupdate"/>
</primary_location>
<schedule AllowLocalConfig="false" SchedEnable="true" Frequency="50" DetectDialUp="false"/>
</AUConfig>
</AUConfigurations>
)sophos";

    ALCPolicy obj{ minPolicy };
    auto settings = obj.getUpdateSettings();
    auto urls = settings.getSophosLocationURLs();
    ASSERT_EQ(urls.size(), 3);
    EXPECT_EQ(urls[0], "http://dci.sophosupd.com/cloudupdate");
    EXPECT_EQ(urls[1], "http://dci.sophosupd.com/update");
    EXPECT_EQ(urls[2], "http://dci.sophosupd.net/update");
}

TEST_F(TestALCPolicy, sdds2_update_server_uses_sophos_alias_file)
{
    const std::string filePath = Common::ApplicationConfiguration::applicationPathManager().getSophosAliasFilePath();
    const std::string expectValue = "Iamaconnectionaddress";

    auto mockFileSystem = std::make_unique<MockFileSystem>();
    EXPECT_CALL(*mockFileSystem, isFile(filePath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, readFile(filePath)).WillOnce(Return(expectValue));
    auto scopedReplaceFileSystem = std::make_unique<Tests::ScopedReplaceFileSystem>(std::move(mockFileSystem));

    constexpr char minPolicy[] = R"sophos(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
  <csc:Comp RevID="b6a8fe2c0ce016c949016a5da2b7a089699271290ef7205d5bea0986768485d9" policyType="1"/>
<AUConfig platform="Linux">
<primary_location>
  <server Algorithm="Clear" UserPassword="xxxxxx" UserName="W2YJXI6FED" ConnectionAddress="http://dci.sophosupd.com/cloudupdate"/>
</primary_location>
<schedule AllowLocalConfig="false" SchedEnable="true" Frequency="50" DetectDialUp="false"/>
</AUConfig>
</AUConfigurations>
)sophos";

    ALCPolicy obj{ minPolicy };
    auto settings = obj.getUpdateSettings();
    auto urls = settings.getSophosLocationURLs();
    ASSERT_EQ(urls.size(), 3);
    EXPECT_EQ(urls[0], expectValue);
    EXPECT_EQ(urls[1], "http://dci.sophosupd.com/update");
    EXPECT_EQ(urls[2], "http://dci.sophosupd.net/update");
}

//Cloud Subscription Tests

TEST_F(TestALCPolicy, cloud_subscriptions)
{
    constexpr char minPolicy[] = R"sophos(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
  <csc:Comp RevID="b6a8fe2c0ce016c949016a5da2b7a089699271290ef7205d5bea0986768485d9" policyType="1"/>
<AUConfig platform="Linux">
    <primary_location>
      <server Algorithm="Clear" UserPassword="xxxxxx" UserName="W2YJXI6FED"/>
    </primary_location>
    <schedule AllowLocalConfig="false" SchedEnable="true" Frequency="50" DetectDialUp="false"/>
    <cloud_subscription RigidName="ServerProtectionLinux-Base" Tag="RECOMMENDED" BaseVersion="10"/>
    <cloud_subscriptions>
      <subscription Id="Base" RigidName="ServerProtectionLinux-Base" Tag="RECOMMENDED" BaseVersion="10" FixedVersion="11"/>
      <subscription Id="Base" RigidName="ServerProtectionLinux-Base9" Tag="RECOMMENDED" BaseVersion="9" FixedVersion="8"/>
    </cloud_subscriptions>
</AUConfig>
</AUConfigurations>
)sophos";

    ALCPolicy obj{ minPolicy };

    {
        auto all_subs = obj.getSubscriptions();
        ASSERT_EQ(all_subs.size(), 2);
        EXPECT_EQ(all_subs[0].rigidName(), "ServerProtectionLinux-Base");
        EXPECT_EQ(all_subs[0].fixedVersion(), "11");
        EXPECT_EQ(all_subs[0].tag(), "RECOMMENDED");
        EXPECT_EQ(all_subs[0].baseVersion(), "10");

        EXPECT_EQ(all_subs[1].rigidName(), "ServerProtectionLinux-Base9");
        EXPECT_EQ(all_subs[1].fixedVersion(), "8");
        EXPECT_EQ(all_subs[1].tag(), "RECOMMENDED");
        EXPECT_EQ(all_subs[1].baseVersion(), "9");
    }
    auto settings = obj.getUpdateSettings();
    auto subs = settings.getProductsSubscription();
    ASSERT_EQ(subs.size(), 1);
    EXPECT_EQ(subs[0].rigidName(), "ServerProtectionLinux-Base9");
    EXPECT_EQ(subs[0].fixedVersion(), "8");
    EXPECT_EQ(subs[0].tag(), "RECOMMENDED");
    EXPECT_EQ(subs[0].baseVersion(), "9");

    auto primary = settings.getPrimarySubscription();
    EXPECT_EQ(primary.rigidName(), "ServerProtectionLinux-Base");
    EXPECT_EQ(primary.fixedVersion(), "11");
    EXPECT_EQ(primary.tag(), "RECOMMENDED");
    EXPECT_EQ(primary.baseVersion(), "10");
}

//Credentials Tests

TEST_F(TestALCPolicy, invalid_credentials_no_user)
{
    constexpr char minPolicy[] = R"sophos(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
  <csc:Comp RevID="b6a8fe2c0ce016c949016a5da2b7a089699271290ef7205d5bea0986768485d9" policyType="1"/>
<AUConfig platform="Linux">
    <primary_location>
      <server Algorithm="Clear" UserPassword="W2YJXI6FEDW2YJXI6FEDW2YJXI6FEDRF" UserName=""/>
    </primary_location>
</AUConfig>
</AUConfigurations>
)sophos";

    try
    {
        ALCPolicy obj{ minPolicy };
        FAIL() << "failed to throw due to no username";
    }
    catch (const PolicyParseException& except)
    {
        EXPECT_STREQ(except.what(), "Invalid policy: Username is empty");
    }
}

TEST_F(TestALCPolicy, invalid_credentials_no_password)
{
    constexpr char minPolicy[] = R"sophos(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
  <csc:Comp RevID="b6a8fe2c0ce016c949016a5da2b7a089699271290ef7205d5bea0986768485d9" policyType="1"/>
<AUConfig platform="Linux">
    <primary_location>
      <server Algorithm="Clear" UserPassword="" UserName="W2YJXI6FEDW2YJXI6FEDW2YJXI6FEDRF"/>
    </primary_location>
</AUConfig>
</AUConfigurations>
)sophos";

    try
    {
        ALCPolicy obj{ minPolicy };
        FAIL() << "failed to throw due to no password";
    }
    catch (const PolicyParseException& except)
    {
        EXPECT_STREQ(except.what(), "Invalid policy: Password is empty");
    }
}

TEST_F(TestALCPolicy, credentials_obfuscated)
{
    constexpr char minPolicy[] = R"sophos(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
  <csc:Comp RevID="b6a8fe2c0ce016c949016a5da2b7a089699271290ef7205d5bea0986768485d9" policyType="1"/>
<AUConfig platform="Linux">
    <primary_location>
      <server Algorithm="Clear" UserPassword="xxxxxx" UserName="W2YJXI6FED"/>
    </primary_location>
</AUConfig>
</AUConfigurations>
)sophos";

    ALCPolicy obj{ minPolicy };

    {
        auto sddsid = obj.getSddsID();
        EXPECT_STREQ(sddsid.c_str(), "W2YJXI6FED");
    }

    auto credentials = obj.getUpdateSettings().getCredentials();
    EXPECT_STREQ(credentials.getPassword().c_str(), "c2d584eb505b6a35fbf2dd9740551fe9");
    EXPECT_STREQ(credentials.getUsername().c_str(), "c2d584eb505b6a35fbf2dd9740551fe9");
}

TEST_F(TestALCPolicy, credentials_deobfuscated_AES256)
{
    constexpr char minPolicy[] = R"sophos(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
  <csc:Comp RevID="b6a8fe2c0ce016c949016a5da2b7a089699271290ef7205d5bea0986768485d9" policyType="1"/>
<AUConfig platform="Linux">
    <primary_location>
      <server Algorithm="AES256" UserPassword="CCDN+JdsRVNd+yKFqQhrmdJ856KCCLHLQxEtgwG/tD5myvTrUk/kuALeUDhL4plxGvM=" UserName="W2YJXI6FED"/>
    </primary_location>
</AUConfig>
</AUConfigurations>
)sophos";

    ALCPolicy obj{ minPolicy };

    {
        auto sddsid = obj.getSddsID();
        EXPECT_STREQ(sddsid.c_str(), "W2YJXI6FED");
    }

    auto credentials = obj.getUpdateSettings().getCredentials();
    EXPECT_STREQ(credentials.getPassword().c_str(), "6e96171819c61fe07361bc1f798d6b46");
    EXPECT_STREQ(credentials.getUsername().c_str(), "6e96171819c61fe07361bc1f798d6b46");
}

TEST_F(TestALCPolicy, credentials_not_deobfuscated)
{
    constexpr char minPolicy[] = R"sophos(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
  <csc:Comp RevID="b6a8fe2c0ce016c949016a5da2b7a089699271290ef7205d5bea0986768485d9" policyType="1"/>
<AUConfig platform="Linux">
    <primary_location>
      <server Algorithm="Clear" UserPassword="W2YJXI6FEDW2YJXI6FEDW2YJXI6FEDRF" UserName="W2YJXI6FEDW2YJXI6FEDW2YJXI6FEDRF"/>
    </primary_location>
</AUConfig>
</AUConfigurations>
)sophos";

    ALCPolicy obj{ minPolicy };

    {
        auto sddsid = obj.getSddsID();
        EXPECT_STREQ(sddsid.c_str(), "W2YJXI6FEDW2YJXI6FEDW2YJXI6FEDRF");
    }

    auto credentials = obj.getUpdateSettings().getCredentials();
    EXPECT_STREQ(credentials.getPassword().c_str(), "W2YJXI6FEDW2YJXI6FEDW2YJXI6FEDRF");
    EXPECT_STREQ(credentials.getUsername().c_str(), "W2YJXI6FEDW2YJXI6FEDW2YJXI6FEDRF");
}

//Proxy Tests

TEST_F(TestALCPolicy, emptyProxy)
{
    constexpr const char policy[] = R"sophos(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
  <csc:Comp RevID="b6a8fe2c0ce016c949016a5da2b7a089699271290ef7205d5bea0986768485d9" policyType="1"/>
<AUConfig platform="Linux">
    <primary_location>
      <server Algorithm="Clear" UserPassword="W2YJXI6FEDW2YJXI6FEDW2YJXI6FEDRF" UserName="W2YJXI6FEDW2YJXI6FEDW2YJXI6FEDRF"/>
      <proxy ProxyType="0" ProxyUserPassword="" ProxyUserName="" ProxyPortNumber="0" ProxyAddress="" AllowLocalConfig="false"/>
    </primary_location>
</AUConfig>
</AUConfigurations>
)sophos";
    ALCPolicy obj{ policy };
    auto settings = obj.getUpdateSettings();
    auto proxy = settings.getPolicyProxy();
    EXPECT_TRUE(proxy.empty());
}

TEST_F(TestALCPolicy, missingProxy)
{
    constexpr const char policy[] = R"sophos(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
  <csc:Comp RevID="b6a8fe2c0ce016c949016a5da2b7a089699271290ef7205d5bea0986768485d9" policyType="1"/>
<AUConfig platform="Linux">
    <primary_location>
      <server Algorithm="Clear" UserPassword="W2YJXI6FEDW2YJXI6FEDW2YJXI6FEDRF" UserName="W2YJXI6FEDW2YJXI6FEDW2YJXI6FEDRF"/>
    </primary_location>
</AUConfig>
</AUConfigurations>
)sophos";
    ALCPolicy obj{ policy };
    auto settings = obj.getUpdateSettings();
    auto proxy = settings.getPolicyProxy();
    EXPECT_TRUE(proxy.empty());
}

TEST_F(TestALCPolicy, proxyType2)
{
    constexpr const char policy[] = R"sophos(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
  <csc:Comp RevID="b6a8fe2c0ce016c949016a5da2b7a089699271290ef7205d5bea0986768485d9" policyType="1"/>
<AUConfig platform="Linux">
    <primary_location>
      <server Algorithm="Clear" UserPassword="W2YJXI6FEDW2YJXI6FEDW2YJXI6FEDRF" UserName="W2YJXI6FEDW2YJXI6FEDW2YJXI6FEDRF"/>
<proxy ProxyType="2" ProxyUserPassword="CCC4Fcz2iNaH44sdmqyLughrajL7svMPTbUZc/Q4c7yAtSrdM03lfO33xI0XKNU4IBY=" ProxyUserName="TestUser" ProxyPortNumber="8080" ProxyAddress="uk-abn-wpan-1.green.sophos" AllowLocalConfig="false"/>
    </primary_location>
</AUConfig>
</AUConfigurations>
)sophos";
    ALCPolicy obj{ policy };
    auto settings = obj.getUpdateSettings();
    auto proxy = settings.getPolicyProxy();
    EXPECT_FALSE(proxy.empty());
    EXPECT_EQ(proxy.getUrl(), "uk-abn-wpan-1.green.sophos:8080");
    const auto& creds = proxy.getCredentials();
    EXPECT_EQ(creds.getUsername(), "TestUser");
    EXPECT_EQ(creds.getDeobfuscatedPassword(), "Ch1pm0nk");
}

//Features Tests

TEST_F(TestALCPolicy, validFeatures)
{
    constexpr const char validFeatures[] = R"sophos(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
  <csc:Comp RevID="b6a8fe2c0ce016c949016a5da2b7a089699271290ef7205d5bea0986768485d9" policyType="1"/>
<AUConfig platform="Linux">
    <primary_location>
      <server Algorithm="Clear" UserPassword="W2YJXI6FEDW2YJXI6FEDW2YJXI6FEDRF" UserName="W2YJXI6FEDW2YJXI6FEDW2YJXI6FEDRF"/>
    </primary_location>
</AUConfig>
<Features>
  <Feature id="CORE"/>
  <Feature id="MTD"/>
</Features>
</AUConfigurations>
)sophos";

    ALCPolicy obj{ validFeatures };
    auto settings = obj.getUpdateSettings();
    auto features = settings.getFeatures();
    ASSERT_EQ(features.size(), 2);
    EXPECT_EQ(features[0], "CORE");
    EXPECT_EQ(features[1], "MTD");
}

TEST_F(TestALCPolicy, coreFeatureAbsent)
{
    constexpr const char validFeatures[] = R"sophos(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
  <csc:Comp RevID="b6a8fe2c0ce016c949016a5da2b7a089699271290ef7205d5bea0986768485d9" policyType="1"/>
<AUConfig platform="Linux">
    <primary_location>
      <server Algorithm="Clear" UserPassword="W2YJXI6FEDW2YJXI6FEDW2YJXI6FEDRF" UserName="W2YJXI6FEDW2YJXI6FEDW2YJXI6FEDRF"/>
    </primary_location>
</AUConfig>
<Features>
  <Feature id="MTD"/>
</Features>
</AUConfigurations>
)sophos";

    UsingMemoryAppender memoryAppenderHolder(*this);
    ALCPolicy obj{ validFeatures };
    auto settings = obj.getUpdateSettings();
    auto features = settings.getFeatures();
    ASSERT_EQ(features.size(), 1);
    EXPECT_EQ(features[0], "MTD");
    EXPECT_TRUE(appenderContains("CORE not in the features of the policy."));
}

TEST_F(TestALCPolicy, missingFeatures)
{
    constexpr const char missingFeatures[] = R"sophos(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
  <csc:Comp RevID="b6a8fe2c0ce016c949016a5da2b7a089699271290ef7205d5bea0986768485d9" policyType="1"/>
<AUConfig platform="Linux">
    <primary_location>
      <server Algorithm="Clear" UserPassword="W2YJXI6FEDW2YJXI6FEDW2YJXI6FEDRF" UserName="W2YJXI6FEDW2YJXI6FEDW2YJXI6FEDRF"/>
    </primary_location>
</AUConfig>
</AUConfigurations>
)sophos";

    UsingMemoryAppender memoryAppenderHolder(*this);
    ALCPolicy obj{ missingFeatures };
    auto settings = obj.getUpdateSettings();
    auto features = settings.getFeatures();
    ASSERT_EQ(features.size(), 0);
    EXPECT_TRUE(appenderContains("CORE not in the features of the policy."));
}

//Period Tests

TEST_F(TestALCPolicy, validPeriod)
{
    constexpr const char validPeriod[] = R"sophos(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
  <csc:Comp RevID="b6a8fe2c0ce016c949016a5da2b7a089699271290ef7205d5bea0986768485d9" policyType="1"/>
<AUConfig platform="Linux">
    <primary_location>
      <server Algorithm="Clear" UserPassword="W2YJXI6FEDW2YJXI6FEDW2YJXI6FEDRF" UserName="W2YJXI6FEDW2YJXI6FEDW2YJXI6FEDRF"/>
    </primary_location>
    <schedule AllowLocalConfig="false" SchedEnable="true" Frequency="23" DetectDialUp="false"/>
</AUConfig>
</AUConfigurations>
)sophos";

    ALCPolicy obj{ validPeriod };
    EXPECT_EQ(obj.getUpdatePeriod(), std::chrono::minutes{23});
}

TEST_F(TestALCPolicy, emptyPeriod)
{
    constexpr const char emptyPeriodPolicy[] = R"sophos(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
  <csc:Comp RevID="b6a8fe2c0ce016c949016a5da2b7a089699271290ef7205d5bea0986768485d9" policyType="1"/>
<AUConfig platform="Linux">
    <primary_location>
      <server Algorithm="Clear" UserPassword="W2YJXI6FEDW2YJXI6FEDW2YJXI6FEDRF" UserName="W2YJXI6FEDW2YJXI6FEDW2YJXI6FEDRF"/>
    </primary_location>
    <schedule AllowLocalConfig="false" SchedEnable="true" Frequency="" DetectDialUp="false"/>
</AUConfig>
</AUConfigurations>
)sophos";

    ALCPolicy obj{ emptyPeriodPolicy };
    EXPECT_EQ(obj.getUpdatePeriod(), std::chrono::minutes{60});
}


TEST_F(TestALCPolicy, missingFrequency)
{
    constexpr const char missingFrequency[] = R"sophos(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
  <csc:Comp RevID="b6a8fe2c0ce016c949016a5da2b7a089699271290ef7205d5bea0986768485d9" policyType="1"/>
<AUConfig platform="Linux">
    <primary_location>
      <server Algorithm="Clear" UserPassword="W2YJXI6FEDW2YJXI6FEDW2YJXI6FEDRF" UserName="W2YJXI6FEDW2YJXI6FEDW2YJXI6FEDRF"/>
    </primary_location>
    <schedule AllowLocalConfig="false" SchedEnable="true" DetectDialUp="false"/>
</AUConfig>
</AUConfigurations>
)sophos";

    ALCPolicy obj{ missingFrequency };
    EXPECT_EQ(obj.getUpdatePeriod(), std::chrono::minutes{60});
}

TEST_F(TestALCPolicy, missingSchedule)
{
    constexpr const char missingSchedule[] = R"sophos(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
  <csc:Comp RevID="b6a8fe2c0ce016c949016a5da2b7a089699271290ef7205d5bea0986768485d9" policyType="1"/>
<AUConfig platform="Linux">
    <primary_location>
      <server Algorithm="Clear" UserPassword="W2YJXI6FEDW2YJXI6FEDW2YJXI6FEDRF" UserName="W2YJXI6FEDW2YJXI6FEDW2YJXI6FEDRF"/>
    </primary_location>
</AUConfig>
</AUConfigurations>
)sophos";

    ALCPolicy obj{ missingSchedule };
    EXPECT_EQ(obj.getUpdatePeriod(), std::chrono::minutes{60});
}

TEST_F(TestALCPolicy, periodNotInt)
{
    constexpr const char periodNotInt[] = R"sophos(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
  <csc:Comp RevID="b6a8fe2c0ce016c949016a5da2b7a089699271290ef7205d5bea0986768485d9" policyType="1"/>
<AUConfig platform="Linux">
    <primary_location>
      <server Algorithm="Clear" UserPassword="W2YJXI6FEDW2YJXI6FEDW2YJXI6FEDRF" UserName="W2YJXI6FEDW2YJXI6FEDW2YJXI6FEDRF"/>
    </primary_location>
    <schedule AllowLocalConfig="false" SchedEnable="true" Frequency="ABCXYZ" DetectDialUp="false"/>
</AUConfig>
</AUConfigurations>
)sophos";

    ALCPolicy obj{ periodNotInt };
    EXPECT_EQ(obj.getUpdatePeriod(), std::chrono::minutes{60});
}

TEST_F(TestALCPolicy, periodTooBig)
{
    constexpr const char periodTooBig[] = R"sophos(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
  <csc:Comp RevID="b6a8fe2c0ce016c949016a5da2b7a089699271290ef7205d5bea0986768485d9" policyType="1"/>
<AUConfig platform="Linux">
    <primary_location>
      <server Algorithm="Clear" UserPassword="W2YJXI6FEDW2YJXI6FEDW2YJXI6FEDRF" UserName="W2YJXI6FEDW2YJXI6FEDW2YJXI6FEDRF"/>
    </primary_location>
    <schedule AllowLocalConfig="false" SchedEnable="true" Frequency="999999999999999999999999999999999999999" DetectDialUp="false"/>
</AUConfig>
</AUConfigurations>
)sophos";

    ALCPolicy obj{ periodTooBig };
    EXPECT_EQ(obj.getUpdatePeriod(), std::chrono::minutes{60});
}

//Update Cache Tests

TEST_F(TestALCPolicy, update_cache)
{
    auto expectedCert = R"(-----BEGIN CERTIFICATE-----
MIIDxDCCAqygAwIBAgIQEFgFEJ0SYXZx+dHI2UTIVzANBgkqhkiG9w0BAQsFADBh
MQswCQYDVQQGEwJHQjEPMA0GA1UEBwwGT3hmb3JkMQ8wDQYDVQQKDAZTb3Bob3Mx
EjAQBgNVBAsMCUhlYXJ0YmVhdDEcMBoGA1UEAwwTSGVhcnRiZWF0LWV1LXdlc3Qt
MTAeFw0xNzExMDkwNzE1MDRaFw0xOTExMTMwNzE1MDRaMIGyMQswCQYDVQQGEwJH
QjEPMA0GA1UEBwwGT3hmb3JkMQ8wDQYDVQQKDAZTb3Bob3MxEjAQBgNVBAsMCUhl
YXJ0YmVhdDEYMBYGA1UEAwwPUHJvZHVjdCBMaWNlbnNlMTgwNgYDVQQDDC9DdXN0
b21lcklEOjRiNGNhM2JhLWMxNDQtNDQ0Ny04MDUwLTZjOTZhNzEwNGMxMTEZMBcG
A1UEAwwQU29waG9zIEhlYXJ0YmVhdDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCC
AQoCggEBAJQSXwmoOsdPRBQUI8f8i7Z6+UhNwvKPoM/kyMbwf2DG8FYPrpiG6uhR
JT7SBRGcWZstWIsafrj+RrXrXjapm0xGqSNc1nIuxQDcXJmmq74XCGedHiQGLF+f
BEDvdYd2KHrYwrg0nGuJB8tPz2s2MRcQLnx30eBL5KAMOkMoZMbkNnCHI8FycTDS
jwjqjN9u3JOMvbA1YEVDonDe39v0Oxj1sY93Oh3xG44496vhhy8r6nwdY9L9DCwn
OU/1wzGapNgDl7ox1JIFIlq+z5iaeGLVp2peVDtPsK8pJBISr6NeG0Mm6wMKkMre
w/Nm8Nu9bV+ocbG/xYncbjP8BYwhkYMCAwEAAaMmMCQwEgYDVR0TAQH/BAgwBgEB
/wIBADAOBgNVHQ8BAf8EBAMCAQYwDQYJKoZIhvcNAQELBQADggEBAFXtbg/c7t1t
MAZFFKUgtkEB5Hh2OBKPS5F87BPfZQD1RLUcbnB+mIuqpAIqHE/3ovtV7KBe0wFz
WgcN08w0Q4yAJV5eNsBZiqZYYRweB157vo/6oY1wvqB6D0tTcfSDG8/kthLITecC
C9jtK5rIwF4pAA4WKkAY6VT+uDCZU+Hejsutc7Ey9Tpon2fCpcmd6HizzCNWeRnG
6YtOsPIjQMToz7b3SnpE2m1zk4jmchQX55yn0rEAf2f/XqblPaEPpoySSNhx/uTg
zhGZIvpcyR9tIr43hyx2iR+E1TH7DqGeeHcbr8pQyd6UAGK9TAA1oDDuejXpQ/GY
c2uueUDEuWQ=
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIIDnTCCAoWgAwIBAgIBAjANBgkqhkiG9w0BAQsFADB/MQswCQYDVQQGEwJHQjEP
MA0GA1UEBwwGT3hmb3JkMQ8wDQYDVQQKDAZTb3Bob3MxFDASBgNVBAsMC0VuZ2lu
ZWVyaW5nMTgwNgYDVQQDDC9Tb3Bob3MgSGVhcnRiZWF0IE5vdC1Gb3ItUmVsZWFz
ZSBSb290IEF1dGhvcml0eTAeFw0xNTA2MDQyMTQyMTNaFw0zNTA2MDQyMTQyMTNa
MGExCzAJBgNVBAYTAkdCMQ8wDQYDVQQHDAZPeGZvcmQxDzANBgNVBAoMBlNvcGhv
czESMBAGA1UECwwJSGVhcnRiZWF0MRwwGgYDVQQDDBNIZWFydGJlYXQtZXUtd2Vz
dC0xMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAlJSxtUjPwyFrldV4
5TJlhP0Kmko3NyHyRqI9xsMRzJ6ZYgJWx1gpPkoT0L4EU+ZStdKbuizL5lhZfBQj
FmxMBLss+tD3WUIE1LQY2Ge+XzbhpTv2QPEslf/nX6T7uEKf3Sb7nsq6JbV3wOjV
R4rsehbUT5tPb0QZAXBu3Cej5ppQrEeQMLeVdU9js8pAKK1HhHJPXcQEjw2SPRGa
S8UnXkK5nd+myQYAgrCtplD0Ul35U2Kc0lRsyVeQsupHlNN5we7HfSI4TrPHox7B
rjAEmQ77c9uP6PbXqY8buo8TlT33eoUYJwFd9ivXutFRf0l8QO2u6Jg+W7eCQqp9
uNYHYwIDAQABo0IwQDAdBgNVHQ4EFgQUrv/9VM4ZJ2TRU4LJ6n3u0YPY8bEwDwYD
VR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMCAQYwDQYJKoZIhvcNAQELBQADggEB
ALIoFNI+pvj57lL83AS1rZX2V6CvGjCiFpR0YfJXlKCMFX9QOm1zT8OKkaLdMAsm
BQZuwhL2k3byk5b8gHYn9IcVE55RC5F2WUsdwke27BIalSSPNitc5sIxKBoc4BLS
2pkhIDwqjTqX9HM31IVT/GlnYqubh1kigqgdOQOIRfCKpb17TZoRQHbC7N5gOoa7
xNwKx/Z1lc6IsUFSFuevVRIhlY0FL0xBIKveMYIBJFgZGY+hRCevqIu1sR0OYNHg
nJwgZ6PRCYPMsEntjoFEy5IoVheP/1yHCC/1seH9nhtGe7xzjuUJHwfPm9kBOpCU
zJqTw7msuYhp6kd4xCw7ccc=
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIIDuzCCAqOgAwIBAgIBATANBgkqhkiG9w0BAQsFADB/MQswCQYDVQQGEwJHQjEP
MA0GA1UEBwwGT3hmb3JkMQ8wDQYDVQQKDAZTb3Bob3MxFDASBgNVBAsMC0VuZ2lu
ZWVyaW5nMTgwNgYDVQQDDC9Tb3Bob3MgSGVhcnRiZWF0IE5vdC1Gb3ItUmVsZWFz
ZSBSb290IEF1dGhvcml0eTAeFw0xNTA2MDQyMTQyMDJaFw0zNTA2MDQyMTQyMDJa
MH8xCzAJBgNVBAYTAkdCMQ8wDQYDVQQHDAZPeGZvcmQxDzANBgNVBAoMBlNvcGhv
czEUMBIGA1UECwwLRW5naW5lZXJpbmcxODA2BgNVBAMML1NvcGhvcyBIZWFydGJl
YXQgTm90LUZvci1SZWxlYXNlIFJvb3QgQXV0aG9yaXR5MIIBIjANBgkqhkiG9w0B
AQEFAAOCAQ8AMIIBCgKCAQEAtQqlMqhJNYlTp/4zElmB7WOEsFUCLVD+Kzneh733
JbpL+RgRQ9qn76+J9vcFPG+eA5zf8M8Mbmb7IBEuLMmD5erT00AwoH8Vr4UP9Wb4
wPn8QzI+WO5oO3NjGvVGD/cufjBH9zqi8oiQwYELrz8fHXTxkCAJHcLp4tM86SQc
QnkWQv9TEWbGgrhJnMg3DAY8pERdLBhjVHPK15ZwyvMlqISBVOm9k8c81fwx4pfG
b4O+1CylPUKqreUXw7T6HB85+ym3TAkq9eeiJA1BkjdhWdpewIII5U955eaWJXaA
ra1jdNb21JAbrwy67Vp4v3uQ+62pOkubThlaQ+qIajJsHwIDAQABo0IwQDAdBgNV
HQ4EFgQUI6jNsBYvHqSsWqJfTrCMizzCHx0wDwYDVR0TAQH/BAUwAwEB/zAOBgNV
HQ8BAf8EBAMCAQYwDQYJKoZIhvcNAQELBQADggEBAE4EtR6dARf1ZqnfQlbjKubN
GGrDVv3GOmPenAH4AcCrXNe6ObIadJFVKWRYMvvMdb4GxxEts29OZdHnFK+Vvrqh
yUV0o4It14BBNjLIApo9eddShyDEZKGusQ33S9XYCIG6Yan59jOo2rvsntwi829v
B5yoGhSoxOaXlExk9YAGXaGmoaJ228iF2vuwmgSHJVxUy02AwiqqEwUy/MGL8877
wFkMtR8hrPVLP0hcHuzWN2cBmrl0C6TeKufqbZBqb/MPn2LWzKcvF44xs3k7uP/H
JWfkv6Tu5jsYGNkN3BSW0x/qjwz7XCSk2ZZxbCgZSq6LpB31sqZctnUxrYSpcdc=
-----END CERTIFICATE-----)";

    ALCPolicy obj{ updatePolicyWithCache };

    auto cacheId = obj.cacheID("2k12-64-ld55-df.eng.sophos:8191");
    EXPECT_EQ(cacheId, "4092822d-0925-4deb-9146-fbc8532f8c55");
    auto certificates = obj.getUpdateCertificatesContent();
    EXPECT_STREQ(certificates.c_str(), expectedCert);

    auto hosts = obj.getUpdateSettings().getLocalUpdateCacheHosts();
    EXPECT_EQ(3, hosts.size());
    EXPECT_EQ(hosts[0], "maineng2.eng.sophos:8191");
    EXPECT_EQ(hosts[1], "2k12-64-ld55-df.eng.sophos:8191");
    EXPECT_EQ(hosts[2], "w2k8r2-std-en-df.eng.sophos:8191");
}

TEST_F(TestALCPolicy, empty_update_cache)
{
    constexpr char no_update_cache_policy[] = R"sophos(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
  <csc:Comp RevID="b6a8fe2c0ce016c949016a5da2b7a089699271290ef7205d5bea0986768485d9" policyType="1"/>
<AUConfig platform="Linux">
<primary_location>
  <server Algorithm="Clear" UserPassword="xxxxxx" UserName="W2YJXI6FED"/>
</primary_location>
</AUConfig>
</AUConfigurations>
)sophos";

    ALCPolicy obj{ no_update_cache_policy };

    auto cacheId = obj.cacheID("2k12-64-ld55-df.eng.sophos:8191");
    EXPECT_STREQ(cacheId.c_str(),"");
    auto certificates = obj.getUpdateCertificatesContent();
    EXPECT_STREQ(certificates.c_str(), "");

    auto hosts = obj.getUpdateSettings().getLocalUpdateCacheHosts();
    EXPECT_EQ(0, hosts.size());
}

//Scheduled Updating/Delay Updating Tests

TEST_F(TestALCPolicy, delay_updating_enabled)
{
    ALCPolicy obj{ updatePolicyWithScheduledUpdate };

    auto schedule = obj.getWeeklySchedule();
    EXPECT_TRUE(schedule.enabled);
    EXPECT_EQ(schedule.hour, 17);
    EXPECT_EQ(schedule.weekDay, 3);
    EXPECT_EQ(schedule.minute, 5);
}

TEST_F(TestALCPolicy, empty_field_delay_updating)
{
    constexpr char empty_day_updating[] = R"sophos(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
  <csc:Comp RevID="b6a8fe2c0ce016c949016a5da2b7a089699271290ef7205d5bea0986768485d9" policyType="1"/>
  <delay_updating Day="" Time="17:00:00"/>
<AUConfig platform="Linux">
<primary_location>
  <server Algorithm="Clear" UserPassword="xxxxxx" UserName="W2YJXI6FED"/>
</primary_location>
</AUConfig>
</AUConfigurations>
)sophos";

    ALCPolicy obj{ empty_day_updating };

    auto schedule = obj.getWeeklySchedule();
    EXPECT_FALSE(schedule.enabled);
    EXPECT_EQ(schedule.hour, 0);
    EXPECT_EQ(schedule.weekDay, 0);
    EXPECT_EQ(schedule.minute, 0);
}

TEST_F(TestALCPolicy, missing_field_delay_updating)
{
    constexpr char missing_day_updating[] = R"sophos(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
  <csc:Comp RevID="b6a8fe2c0ce016c949016a5da2b7a089699271290ef7205d5bea0986768485d9" policyType="1"/>
  <delay_updating Time="17:00:00"/>
<AUConfig platform="Linux">
<primary_location>
  <server Algorithm="Clear" UserPassword="xxxxxx" UserName="W2YJXI6FED"/>
</primary_location>
</AUConfig>
</AUConfigurations>
)sophos";

    ALCPolicy obj{ missing_day_updating };

    auto schedule = obj.getWeeklySchedule();
    EXPECT_FALSE(schedule.enabled);
    EXPECT_EQ(schedule.hour, 0);
    EXPECT_EQ(schedule.weekDay, 0);
    EXPECT_EQ(schedule.minute, 0);
}

TEST_F(TestALCPolicy, no_delay_updating)
{
    constexpr char no_day_updating[] = R"sophos(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
  <csc:Comp RevID="b6a8fe2c0ce016c949016a5da2b7a089699271290ef7205d5bea0986768485d9" policyType="1"/>
<AUConfig platform="Linux">
<primary_location>
  <server Algorithm="Clear" UserPassword="xxxxxx" UserName="W2YJXI6FED"/>
</primary_location>
</AUConfig>
</AUConfigurations>
)sophos";

    ALCPolicy obj{ no_day_updating };

    auto schedule = obj.getWeeklySchedule();
    EXPECT_FALSE(schedule.enabled);
    EXPECT_EQ(schedule.hour, 0);
    EXPECT_EQ(schedule.weekDay, 0);
    EXPECT_EQ(schedule.minute, 0);
}

TEST_F(TestALCPolicy, getTelemetry)
{
    const std::string policy=R"(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
  <csc:Comp RevID="8980d3ddce5f4b4e5911ddb617e8a9e440ba78da8c288312900847bb75737628" policyType="1"/>
  <AUConfig platform="Linux">
    <primary_location>
      <server BandwidthLimit="256" AutoDial="false" Algorithm="AES256" UserPassword="CCC9XRvgRuGLpCpmE+LX3N+7Whc41czNucxdwpVa4wuJVQJLTQN1/oxMZJMkb3qYfT8=" UserName="CSP7I0S0GZZE" UseSophos="true" UseHttps="true" UseDelta="true" ConnectionAddress="http://dci.sophosupd.com/cloudupdate" AllowLocalConfig="false"/>
      <proxy ProxyType="0" ProxyUserPassword="" ProxyUserName="" ProxyPortNumber="0" ProxyAddress="" AllowLocalConfig="false"/>
    </primary_location>
    <cloud_subscription RigidName="ServerProtectionLinux-Base" Tag="RECOMMENDED"/>
    <cloud_subscriptions>
      <subscription Id="Base" RigidName="ServerProtectionLinux-Base" Tag="RECOMMENDED"/>
    </cloud_subscriptions>
  </AUConfig>
  <Features>
    <Feature id="CORE"/>
  </Features>
  <server_names>
    <telemetry>t1.sophosupd.com</telemetry>
</server_names>
</AUConfigurations>)";
    ALCPolicy obj{ policy };
    EXPECT_EQ(obj.getTelemetryHost(), "t1.sophosupd.com");
}