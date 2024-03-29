// Copyright 2018-2024 Sophos Limited. All rights reserved.

#include "Common/Logging/ConsoleLoggingSetup.h"
#include "Common/OSUtilitiesImpl/DnsLookupImpl.h"
#include "Common/OSUtilitiesImpl/LocalIPImpl.h"
#include "Common/XmlUtilities/AttributesMap.h"
#include "UpdateSchedulerImpl/configModule/UpdatePolicyTranslator.h"
#include <gmock/gmock-matchers.h>
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/MockFileSystem.h"
#include "tests/Common/OSUtilitiesImpl/MockDnsLookup.h"
#include "tests/Common/OSUtilitiesImpl/MockILocalIP.h"
#include "Common/Policy/PolicyParseException.h"

#include <future>


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
        <fixed_version>
            <token/>
            <name/>
        </fixed_version>
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
<server_names>
  <sdds3>
    <sus>sus.sophosupd.com</sus>
    <content_servers>
      <server>sdds3test.sophosupd.com</server>
      <server>sdds3test.sophosupd.net</server>
    </content_servers>
  </sdds3>
  <telemetry>t1.sophosupd.com</telemetry>
  <sdu>
    <feedback>sdu-feedback.sophos.com</feedback>
    <repairkit>sdu-auto-upload.sophosupd.com</repairkit>
  </sdu>
</server_names>
</AUConfigurations>
)sophos" };


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
        <fixed_version>
            <token/>
            <name/>
        </fixed_version>
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
<server_names>
  <sdds3>
    <sus>sus.sophosupd.com</sus>
    <content_servers>
      <server>sdds3test.sophosupd.com</server>
      <server>sdds3test.sophosupd.net</server>
    </content_servers>
  </sdds3>
  <telemetry>t1.sophosupd.com</telemetry>
  <sdu>
    <feedback>sdu-feedback.sophos.com</feedback>
    <repairkit>sdu-auto-upload.sophosupd.com</repairkit>
  </sdu>
</server_names>
</AUConfigurations>
)sophos" };


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
        <fixed_version>
            <token/>
            <name/>
        </fixed_version>
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


static const std::string incorrectPolicyTypeXml{ R"sophos(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
  <csc:Comp RevID="f6babe12a13a5b2134c5861d01aed0eaddc20ea374e3a717ee1ea1451f5e2cf6" policyType="2"/>
</AUConfigurations>
)sophos" };


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
        <fixed_version>
            <token/>
            <name/>
        </fixed_version>
    <delay_supplements enabled="true"/>
  </AUConfig>
  <Features>
    <Feature id="CORE"/>
    <Feature id="SDU"/>
    <Feature id="MDR"/>
  </Features>
  <intelligent_updating Enabled="false" SubscriptionPolicy="2DD71664-8D18-42C5-B3A0-FF0D289265BF"/>
  <customer id="8dd8c9f3-a9a1-84e2-49d8-f9320a76298e"/>
<server_names>
  <sdds3>
    <sus>sus.sophosupd.com</sus>
    <content_servers>
      <server>sdds3test.sophosupd.com</server>
      <server>sdds3test.sophosupd.net</server>
    </content_servers>
  </sdds3>
  <telemetry>t1.sophosupd.com</telemetry>
  <sdu>
    <feedback>sdu-feedback.sophos.com</feedback>
    <repairkit>sdu-auto-upload.sophosupd.com</repairkit>
  </sdu>
</server_names>
</AUConfigurations>
)sophos" };

namespace
{
    std::string replaceXMLSection(const std::string& xml, const std::string& section, std::string replacement = "")
    {
        if (replacement.empty())
        {
            replacement = "<" + section + "/>";
        }
        std::string sectionStart = "<" + section + ">";
        std::string sectionEnd = "</" + section + ">\n";

        size_t posStart = xml.find(sectionStart);
        size_t posEnd = xml.find(sectionEnd) + sectionEnd.size();
        std::string editedXml = xml.substr(0, posStart) + replacement + "\n" + xml.substr(posEnd);
        return editedXml;
    }

} // namespace

using namespace Common::Policy;
using namespace UpdateSchedulerImpl::configModule;

class TestUpdatePolicyTranslator : public ::testing::Test
{
public:
    TestUpdatePolicyTranslator() : m_loggingSetup() {}
private:
    Common::Logging::ConsoleLoggingSetup m_loggingSetup;

};

TEST_F(TestUpdatePolicyTranslator, ParseUpdatePolicyWithUpdateCache)
{
    std::unique_ptr<FakeILocalIP> fakeILocalIP(
        new FakeILocalIP(std::vector<std::string>{ "192.168.10.5", "10.10.5.6" }));
    Common::OSUtilitiesImpl::replaceLocalIP(std::move(fakeILocalIP));
    // do not provide ip for the update caches, should keep the order
    std::unique_ptr<FakeIDnsLookup> fake(new FakeIDnsLookup());
    Common::OSUtilitiesImpl::replaceDnsLookup(std::move(fake));

    auto* mockFileSystem = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*mockFileSystem, isFile(_)).WillRepeatedly(Return(false));
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));

    UpdatePolicyTranslator translator;

    auto settingsHolder = translator.translatePolicy(updatePolicyWithCache);
    auto config = settingsHolder.configurationData;

    EXPECT_EQ(settingsHolder.updateCacheCertificatesContent, cacheCertificates);

    ASSERT_EQ(config.getInstallArguments().size(), 2);
    EXPECT_EQ(config.getInstallArguments()[0], "--instdir");
    EXPECT_EQ(config.getInstallArguments()[1], "/opt/sophos-spl");

    auto urls = config.getSophosCDNURLs();
    ASSERT_EQ(urls.size(), 2);
    EXPECT_EQ(urls[0], "https://sdds3test.sophosupd.com");
    EXPECT_EQ(urls[1], "https://sdds3test.sophosupd.net");

    auto cacheUrls = config.getLocalUpdateCacheHosts();
    ASSERT_EQ(cacheUrls.size(), 3);
    EXPECT_EQ(cacheUrls[0], "maineng2.eng.sophos:8191");
    EXPECT_EQ(cacheUrls[1], "2k12-64-ld55-df.eng.sophos:8191");
    EXPECT_EQ(cacheUrls[2], "w2k8r2-std-en-df.eng.sophos:8191");

    const auto& primarySubscription = config.getPrimarySubscription();
    EXPECT_EQ(primarySubscription.baseVersion(), "10");
    EXPECT_EQ(primarySubscription.rigidName(), "ServerProtectionLinux-Base");
    EXPECT_EQ(primarySubscription.tag(), "RECOMMENDED");
    EXPECT_EQ(primarySubscription.fixedVersion(), "11");

    // Won't include the ServerProtectionLinux-Base subscription
    const auto& productsSubscription = config.getProductsSubscription();
    ASSERT_EQ(productsSubscription.size(), 1);
    EXPECT_EQ(productsSubscription[0].baseVersion(), "9");
    EXPECT_EQ(productsSubscription[0].rigidName(), "ServerProtectionLinux-Base9");
    EXPECT_EQ(productsSubscription[0].tag(), "RECOMMENDED");
    EXPECT_EQ(productsSubscription[0].fixedVersion(), "8");

    const auto& features = config.getFeatures();
    std::vector<std::string> expectedFeatures = { "APPCNTRL", "AV",  "CORE", "DLP", "DVCCNTRL", "EFW",
                                                  "HBT",      "MTD", "NTP",  "SAV", "SDU",      "WEBCNTRL" };
    EXPECT_EQ(features, expectedFeatures);

    EXPECT_TRUE(config.getPolicyProxy().empty());

    EXPECT_EQ(settingsHolder.schedulerPeriod, std::chrono::minutes(50));
    EXPECT_EQ(settingsHolder.weeklySchedule.enabled, false);
    Common::OSUtilitiesImpl::restoreDnsLookup();
    Common::OSUtilitiesImpl::restoreLocalIP();
}

TEST_F(TestUpdatePolicyTranslator, TranslatorHandlesCacheIDAndRevID)
{
    auto* mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));

    UpdatePolicyTranslator translator;

    EXPECT_CALL(*mockFileSystem, isFile(_)).WillRepeatedly(Return(false));

    auto settingsHolder = translator.translatePolicy(updatePolicyWithCache);
    auto config = settingsHolder.configurationData;

    EXPECT_EQ(translator.cacheID("maineng2.eng.sophos:8191"), "d4739ed5-b66a-4a21-9218-4fd0dfe4dcbd");
    EXPECT_EQ(translator.cacheID("https://maineng2.eng.sophos:8191/v3"), "d4739ed5-b66a-4a21-9218-4fd0dfe4dcbd");
    EXPECT_TRUE(translator.cacheID("invalid_cache").empty());
    EXPECT_EQ(translator.revID(), "b6a8fe2c0ce016c949016a5da2b7a089699271290ef7205d5bea0986768485d9");
}

TEST_F(TestUpdatePolicyTranslator, ParseUpdatePolicyWithProxy)
{
    auto* mockFileSystem = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*mockFileSystem, isFile(_)).WillRepeatedly(Return(false));
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));

    UpdatePolicyTranslator translator;


    auto settingsHolder = translator.translatePolicy(updatePolicyWithProxy);
    auto config = settingsHolder.configurationData;

    EXPECT_TRUE(settingsHolder.updateCacheCertificatesContent.empty());


    ASSERT_EQ(config.getInstallArguments().size(), 2);
    EXPECT_EQ(config.getInstallArguments()[0], "--instdir");
    EXPECT_EQ(config.getInstallArguments()[1], "/opt/sophos-spl");

    auto urls = config.getSophosCDNURLs();
    ASSERT_EQ(urls.size(), 2);
    EXPECT_EQ(urls[0], "https://sdds3test.sophosupd.com");
    EXPECT_EQ(urls[1], "https://sdds3test.sophosupd.net");

    EXPECT_TRUE(config.getLocalUpdateCacheHosts().empty());

    const auto& primarySubscription = config.getPrimarySubscription();
    EXPECT_EQ(primarySubscription.baseVersion(), "10");
    EXPECT_EQ(primarySubscription.rigidName(), "ServerProtectionLinux-Base");
    EXPECT_EQ(primarySubscription.tag(), "RECOMMENDED");
    EXPECT_EQ(primarySubscription.fixedVersion(), "");

    const auto& productsSubscription = config.getProductsSubscription();
    ASSERT_EQ(productsSubscription.size(), 1);
    EXPECT_EQ(productsSubscription[0].baseVersion(), "9");
    EXPECT_EQ(productsSubscription[0].rigidName(), "ServerProtectionLinux-Base9");
    EXPECT_EQ(productsSubscription[0].tag(), "RECOMMENDED");
    EXPECT_EQ(productsSubscription[0].fixedVersion(), "");

    const auto& features = config.getFeatures();
    std::vector<std::string> expectedFeatures = { "APPCNTRL", "AV",  "CORE", "DLP", "DVCCNTRL", "EFW",
                                                  "HBT",      "MTD", "NTP",  "SAV", "SDU",      "WEBCNTRL" };
    EXPECT_EQ(features, expectedFeatures);

    Proxy expectedProxy{
        "uk-abn-wpan-1.green.sophos:8080",
        ProxyCredentials{
            "TestUser", "CCC4Fcz2iNaH44sdmqyLughrajL7svMPTbUZc/Q4c7yAtSrdM03lfO33xI0XKNU4IBY=", "2" }
    };
    EXPECT_EQ(config.getPolicyProxy(), expectedProxy);  EXPECT_EQ(settingsHolder.schedulerPeriod, std::chrono::minutes(40));
    EXPECT_EQ(settingsHolder.weeklySchedule.enabled, false);
}

TEST_F(TestUpdatePolicyTranslator, ParseUpdatePolicyWithScheduledUpdate)
{
    auto* mockFileSystem = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*mockFileSystem, isFile(_)).WillRepeatedly(Return(false));
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));

    UpdatePolicyTranslator translator;


    auto settingsHolder = translator.translatePolicy(updatePolicyWithScheduledUpdate);
    auto config = settingsHolder.configurationData;

    const auto& primarySubscription = config.getPrimarySubscription();
    EXPECT_EQ(primarySubscription.baseVersion(), "10");
    EXPECT_EQ(primarySubscription.rigidName(), "ServerProtectionLinux-Base");
    EXPECT_EQ(primarySubscription.tag(), "RECOMMENDED");
    EXPECT_EQ(primarySubscription.fixedVersion(), "");

    const auto& productsSubscription = config.getProductsSubscription();
    ASSERT_EQ(productsSubscription.size(), 1);
    EXPECT_EQ(productsSubscription[0].baseVersion(), "9");
    EXPECT_EQ(productsSubscription[0].rigidName(), "ServerProtectionLinux-Base9");
    EXPECT_EQ(productsSubscription[0].tag(), "RECOMMENDED");
    EXPECT_EQ(productsSubscription[0].fixedVersion(), "");

    const auto& features = config.getFeatures();
    std::vector<std::string> expectedFeatures = { "APPCNTRL", "AV",  "CORE", "DLP", "DVCCNTRL", "EFW",
                                                  "HBT",      "MTD", "NTP",  "SAV", "SDU",      "WEBCNTRL" };
    EXPECT_EQ(features, expectedFeatures);

    auto schedule = settingsHolder.weeklySchedule;
    EXPECT_EQ(schedule.enabled, true);
    EXPECT_EQ(schedule.weekDay, 3);
    EXPECT_EQ(schedule.hour, 17);
    EXPECT_EQ(schedule.minute, 0);
}

TEST_F(TestUpdatePolicyTranslator, ParseUpdatePolicyWithScheduledUpdateButMissingTimeField)
{
    auto* mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));
    UpdatePolicyTranslator translator;
    EXPECT_CALL(*mockFileSystem, isFile(_)).WillRepeatedly(Return(false));
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
            <delay_updating Day="Wednesday" />
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
    auto settingsHolder = translator.translatePolicy(updatePolicyWithScheduledUpdate);
    auto config = settingsHolder.configurationData;

    const auto& primarySubscription = config.getPrimarySubscription();
    EXPECT_EQ(primarySubscription.baseVersion(), "10");
    EXPECT_EQ(primarySubscription.rigidName(), "ServerProtectionLinux-Base");
    EXPECT_EQ(primarySubscription.tag(), "RECOMMENDED");
    EXPECT_EQ(primarySubscription.fixedVersion(), "");

    const auto& productsSubscription = config.getProductsSubscription();
    ASSERT_EQ(productsSubscription.size(), 1);
    EXPECT_EQ(productsSubscription[0].baseVersion(), "9");
    EXPECT_EQ(productsSubscription[0].rigidName(), "ServerProtectionLinux-Base9");
    EXPECT_EQ(productsSubscription[0].tag(), "RECOMMENDED");
    EXPECT_EQ(productsSubscription[0].fixedVersion(), "");

    const auto& features = config.getFeatures();
    std::vector<std::string> expectedFeatures = { "APPCNTRL", "AV",  "CORE", "DLP", "DVCCNTRL", "EFW",
                                                  "HBT",      "MTD", "NTP",  "SAV", "SDU",      "WEBCNTRL" };
    EXPECT_EQ(features, expectedFeatures);

    // We expect the schedule to be disabled because the policy did not contain both date and time.
    auto schedule = settingsHolder.weeklySchedule;
    EXPECT_EQ(schedule.enabled, false);
    EXPECT_EQ(schedule.weekDay, 0);
    EXPECT_EQ(schedule.hour, 0);
    EXPECT_EQ(schedule.minute, 0);
}

TEST_F(TestUpdatePolicyTranslator, TelemetryIsCorrectAndRetrievingTelemetryStillGetsTheCorrectData)
{
    auto* mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));
    EXPECT_CALL(*mockFileSystem, isFile(_)).WillRepeatedly(Return(false));

    UpdatePolicyTranslator translator;
    (void)translator.translatePolicy(updatePolicyWithScheduledUpdate);
    std::string expectedTelemetry{
        R"sophos({"esmName":"","esmToken":"","subscriptions-ServerProtectionLinux-Base":"RECOMMENDED","subscriptions-ServerProtectionLinux-Base9":"RECOMMENDED"})sophos"
    };
    std::string telemetry = Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
    EXPECT_EQ(telemetry, expectedTelemetry);
    // second time with additional 'extra' value
    Common::Telemetry::TelemetryHelper::getInstance().set("extra", "newvalue");
    std::string expectedTelemetryWithExtra{
        R"sophos({"esmName":"","esmToken":"","extra":"newvalue","subscriptions-ServerProtectionLinux-Base":"RECOMMENDED","subscriptions-ServerProtectionLinux-Base9":"RECOMMENDED"})sophos"
    };
    telemetry = Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
    EXPECT_EQ(telemetry, expectedTelemetryWithExtra);
    // third time telemetry the 'extra' value is gone
    telemetry = Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
    EXPECT_EQ(telemetry, expectedTelemetry);
}

TEST_F(TestUpdatePolicyTranslator, TelemetryWithFixedVersionNotEmpty)
{
    auto* mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));
    EXPECT_CALL(*mockFileSystem, isFile(_)).WillRepeatedly(Return(false));

    UpdatePolicyTranslator translator;
    (void)translator.translatePolicy(updatePolicyWithCache);
    std::string expectedTelemetry{
        R"sophos({"esmName":"","esmToken":"","subscriptions-ServerProtectionLinux-Base":"11","subscriptions-ServerProtectionLinux-Base9":"8"})sophos"
    };
    std::string telemetry = Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
    EXPECT_EQ(telemetry, expectedTelemetry);
}

TEST_F(TestUpdatePolicyTranslator, TelemetryWithFixedVersionCallSerialiseAndResetKeepsExpectedData)
{
    auto* mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));
    EXPECT_CALL(*mockFileSystem, isFile(_)).WillRepeatedly(Return(false));

    UpdatePolicyTranslator translator;
    (void)translator.translatePolicy(updatePolicyWithCache);
    std::string expectedTelemetry{
        R"sophos({"esmName":"","esmToken":"","subscriptions-ServerProtectionLinux-Base":"11","subscriptions-ServerProtectionLinux-Base9":"8"})sophos"
    };
    std::string telemetry1 = Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
    std::string telemetry2 = Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
    EXPECT_EQ(telemetry1, expectedTelemetry);
    EXPECT_EQ(telemetry2, expectedTelemetry);
}

TEST_F(TestUpdatePolicyTranslator, ParseIncorrectUpdatePolicyType)
{
    auto* mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));

    UpdatePolicyTranslator translator;

    EXPECT_CALL(*mockFileSystem, isFile(_)).WillRepeatedly(Return(false));

    EXPECT_THROW(translator.translatePolicy(incorrectPolicyTypeXml), std::runtime_error);
}

TEST_F(TestUpdatePolicyTranslator, SortUpdateCacheEntries1)
{
    std::unique_ptr<FakeILocalIP> fakeILocalIP(
        new FakeILocalIP(std::vector<std::string>{ "192.168.10.5", "10.10.5.6" }));
    Common::OSUtilitiesImpl::replaceLocalIP(std::move(fakeILocalIP));
    // do not provide ip for the update caches, should keep the order
    std::unique_ptr<FakeIDnsLookup> fake(new FakeIDnsLookup());

    fake->addMap(
        "maineng2.eng.sophos", { { "200.10.1.20" } }); // make it far away, but it will be first as it has priority 0
    fake->addMap("2k12-64-ld55-df.eng.sophos", { { "192.168.1.1" } });
    fake->addMap(
        "w2k8r2-std-en-df.eng.sophos", { { "192.168.10.1" } }); // make this the closest one so it will jump the queue.

    Common::OSUtilitiesImpl::replaceDnsLookup(std::move(fake));

    auto* mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));

    UpdatePolicyTranslator translator;

    EXPECT_CALL(*mockFileSystem, isFile(_)).WillRepeatedly(Return(false));

    auto settingsHolder = translator.translatePolicy(updatePolicyWithCache);
    auto config = settingsHolder.configurationData;

    auto cacheUrls = config.getLocalUpdateCacheHosts();
    ASSERT_EQ(cacheUrls.size(), 3);
    EXPECT_EQ(cacheUrls[0], "maineng2.eng.sophos:8191");
    EXPECT_EQ(cacheUrls[1], "w2k8r2-std-en-df.eng.sophos:8191");
    EXPECT_EQ(cacheUrls[2], "2k12-64-ld55-df.eng.sophos:8191");

    Common::OSUtilitiesImpl::restoreDnsLookup();
    Common::OSUtilitiesImpl::restoreLocalIP();
}

TEST_F(TestUpdatePolicyTranslator, SortUpdateCacheEntries2)
{
    std::unique_ptr<FakeILocalIP> fakeILocalIP(
        new FakeILocalIP(std::vector<std::string>{ "192.168.10.5", "10.10.5.6" }));
    Common::OSUtilitiesImpl::replaceLocalIP(std::move(fakeILocalIP));
    // do not provide ip for the update caches, should keep the order
    std::unique_ptr<FakeIDnsLookup> fake(new FakeIDnsLookup());

    fake->addMap(
        "maineng2.eng.sophos", { { "200.10.1.20" } }); // make it far away, but it will be first as it has priority 0
    fake->addMap("2k12-64-ld55-df.eng.sophos", { { "192.168.10.1" } }); // make this the closest
    fake->addMap("w2k8r2-std-en-df.eng.sophos", { { "192.168.1.1" } });

    Common::OSUtilitiesImpl::replaceDnsLookup(std::move(fake));

    auto* mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));

    UpdatePolicyTranslator translator;

    EXPECT_CALL(*mockFileSystem, isFile(_)).WillRepeatedly(Return(false));

    auto settingsHolder = translator.translatePolicy(updatePolicyWithCache);
    auto config = settingsHolder.configurationData;

    auto cacheUrls = config.getLocalUpdateCacheHosts();
    ASSERT_EQ(cacheUrls.size(), 3);
    EXPECT_EQ(cacheUrls[0], "maineng2.eng.sophos:8191");
    EXPECT_EQ(cacheUrls[1], "2k12-64-ld55-df.eng.sophos:8191");
    EXPECT_EQ(cacheUrls[2], "w2k8r2-std-en-df.eng.sophos:8191");

    Common::OSUtilitiesImpl::restoreDnsLookup();
    Common::OSUtilitiesImpl::restoreLocalIP();
}

TEST_F(TestUpdatePolicyTranslator, ParseMDRPolicy)
{
    auto* mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));

    UpdatePolicyTranslator translator;

    EXPECT_CALL(*mockFileSystem, isFile(_)).WillRepeatedly(Return(false));

    auto settingsHolder = translator.translatePolicy(mdrSSPLBasePolicy);
    auto config = settingsHolder.configurationData;

    EXPECT_TRUE(settingsHolder.updateCacheCertificatesContent.empty());

    ASSERT_EQ(config.getInstallArguments().size(), 2);
    EXPECT_EQ(config.getInstallArguments()[0], "--instdir");
    EXPECT_EQ(config.getInstallArguments()[1], "/opt/sophos-spl");

    auto urls = config.getSophosCDNURLs();
    ASSERT_EQ(urls.size(), 2);
    EXPECT_EQ(urls[0], "https://sdds3test.sophosupd.com");
    EXPECT_EQ(urls[1], "https://sdds3test.sophosupd.net");

    EXPECT_TRUE(config.getLocalUpdateCacheHosts().empty());

    const auto& primarySubscription = config.getPrimarySubscription();
    EXPECT_EQ(primarySubscription.baseVersion(), "");
    EXPECT_EQ(primarySubscription.rigidName(), "ServerProtectionLinux-Base");
    EXPECT_EQ(primarySubscription.tag(), "RECOMMENDED");
    EXPECT_EQ(primarySubscription.fixedVersion(), "");

    const auto& productsSubscription = config.getProductsSubscription();
    ASSERT_EQ(productsSubscription.size(), 1);
    EXPECT_EQ(productsSubscription[0].baseVersion(), "");
    EXPECT_EQ(productsSubscription[0].rigidName(), "ServerProtectionLinux-Plugin-MDR");
    EXPECT_EQ(productsSubscription[0].tag(), "RECOMMENDED");
    EXPECT_EQ(productsSubscription[0].fixedVersion(), "");

    const auto& features = config.getFeatures();
    std::vector<std::string> expectedFeatures = { "CORE", "SDU", "MDR" };
    EXPECT_EQ(features, expectedFeatures);

    EXPECT_EQ(config.getPolicyProxy(), Proxy());
    EXPECT_EQ(settingsHolder.schedulerPeriod, std::chrono::minutes(60));
    auto schedule = settingsHolder.weeklySchedule;
    EXPECT_EQ(schedule.enabled, true);
    EXPECT_EQ(schedule.weekDay, 3);
    EXPECT_EQ(schedule.hour, 11);
    EXPECT_EQ(schedule.minute, 0);
}


TEST_F(TestUpdatePolicyTranslator, ParseMDRPolicyWithNoFeaturesReportsErrorInLog)
{
    auto* mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));

    UpdatePolicyTranslator translator;

    EXPECT_CALL(*mockFileSystem, isFile(_)).WillRepeatedly(Return(false));

    std::string policy = replaceXMLSection(mdrSSPLBasePolicy, "Features");

    testing::internal::CaptureStderr();
    auto settingsHolder = translator.translatePolicy(policy);
    auto config = settingsHolder.configurationData;

    const auto& primarySubscription = config.getPrimarySubscription();
    EXPECT_EQ(primarySubscription.baseVersion(), "");
    EXPECT_EQ(primarySubscription.rigidName(), "ServerProtectionLinux-Base");
    EXPECT_EQ(primarySubscription.tag(), "RECOMMENDED");
    EXPECT_EQ(primarySubscription.fixedVersion(), "");

    const auto& productsSubscription = config.getProductsSubscription();
    ASSERT_EQ(productsSubscription.size(), 1);
    EXPECT_EQ(productsSubscription[0].baseVersion(), "");
    EXPECT_EQ(productsSubscription[0].rigidName(), "ServerProtectionLinux-Plugin-MDR");
    EXPECT_EQ(productsSubscription[0].tag(), "RECOMMENDED");
    EXPECT_EQ(productsSubscription[0].fixedVersion(), "");

    const auto& features = config.getFeatures();
    ASSERT_EQ(features.size(), 0);

    std::string errorMsg = testing::internal::GetCapturedStderr();
    EXPECT_THAT(errorMsg, ::testing::HasSubstr("CORE not in the features of the policy"));
}

TEST_F(TestUpdatePolicyTranslator, ParseMDRPolicyWithFeaturesNotIncludingCoreReportsErrorInLog)
{
    auto* mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));

    UpdatePolicyTranslator translator;

    std::string extraFeatures{ R"sophos(  <Features>
    <Feature id="APPCNTRL"/>
    <Feature id="AV"/>
    <Feature id="DLP"/>
    <Feature id="DVCCNTRL"/>
    <Feature id="EFW"/>
    <Feature id="HBT"/>
    <Feature id="MTD"/>
    <Feature id="NTP"/>
    <Feature id="SAV"/>
    <Feature id="SDU"/>
    <Feature id="WEBCNTRL"/>
  </Features>)sophos" };
    std::string policy = replaceXMLSection(mdrSSPLBasePolicy, "Features", extraFeatures);

    testing::internal::CaptureStderr();

    EXPECT_CALL(*mockFileSystem, isFile(_)).WillRepeatedly(Return(false));

    auto settingsHolder = translator.translatePolicy(policy);
    auto config = settingsHolder.configurationData;

    const auto& primarySubscription = config.getPrimarySubscription();
    EXPECT_EQ(primarySubscription.baseVersion(), "");
    EXPECT_EQ(primarySubscription.rigidName(), "ServerProtectionLinux-Base");
    EXPECT_EQ(primarySubscription.tag(), "RECOMMENDED");
    EXPECT_EQ(primarySubscription.fixedVersion(), "");

    const auto& productsSubscription = config.getProductsSubscription();
    ASSERT_EQ(productsSubscription.size(), 1);
    EXPECT_EQ(productsSubscription[0].baseVersion(), "");
    EXPECT_EQ(productsSubscription[0].rigidName(), "ServerProtectionLinux-Plugin-MDR");
    EXPECT_EQ(productsSubscription[0].tag(), "RECOMMENDED");
    EXPECT_EQ(productsSubscription[0].fixedVersion(), "");

    const auto& features = config.getFeatures();
    std::vector<std::string> expectedFeatures = { "APPCNTRL", "AV",  "DLP", "DVCCNTRL", "EFW",     "HBT",
                                                  "MTD",      "NTP", "SAV", "SDU",      "WEBCNTRL" };
    EXPECT_EQ(features, expectedFeatures);

    std::string errorMsg = testing::internal::GetCapturedStderr();
    EXPECT_THAT(errorMsg, ::testing::HasSubstr("CORE not in the features of the policy"));
}

TEST_F(TestUpdatePolicyTranslator, ParseMDRPolicyWithNoSubscriptionsReportsErrorInLog)
{
    auto* mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));

    UpdatePolicyTranslator translator;

    EXPECT_CALL(*mockFileSystem, isFile(_)).WillRepeatedly(Return(false));

    auto policy = replaceXMLSection(mdrSSPLBasePolicy, "cloud_subscriptions");

    testing::internal::CaptureStderr();
    EXPECT_THROW(translator.translatePolicy(policy),Common::Exceptions::IException);

    std::string errorMsg = testing::internal::GetCapturedStderr();
    EXPECT_THAT(
        errorMsg,
        ::testing::HasSubstr(
            "SSPL base product name : ServerProtectionLinux-Base not in the subscription of the policy"));
}

TEST_F(TestUpdatePolicyTranslator, ParseMDRPolicyWithNoBaseSubscriptionReportsErrorInLog)
{
    auto* mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));

    UpdatePolicyTranslator translator;

    std::string subscriptionWithoutBase{ R"sophos(    <cloud_subscriptions>
      <subscription Id="NotBase" RigidName="ServerProtectionLinux-NotBase" Tag="RECOMMENDED"/>
      <subscription Id="MDR" RigidName="ServerProtectionLinux-Plugin-MDR" Tag="RECOMMENDED"/>
    </cloud_subscriptions>)sophos" };

    auto policy = replaceXMLSection(mdrSSPLBasePolicy, "cloud_subscriptions", subscriptionWithoutBase);

    testing::internal::CaptureStderr();

    EXPECT_CALL(*mockFileSystem, isFile(_)).WillRepeatedly(Return(false));


    EXPECT_THROW(translator.translatePolicy(policy),Common::Exceptions::IException);

    std::string errorMsg = testing::internal::GetCapturedStderr();
    EXPECT_THAT(
        errorMsg,
        ::testing::HasSubstr(
            "SSPL base product name : ServerProtectionLinux-Base not in the subscription of the policy"));
}


TEST_F(TestUpdatePolicyTranslator, ParsePolicyWithSlowSupplements)
{

    const std::string policy = R"sophos(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
  <csc:Comp RevID="f6babe12a13a5b2134c5861d01aed0eaddc20ea374e3a717ee1ea1451f5e2cf6" policyType="1"/>
  <AUConfig platform="Linux">
<cloud_subscriptions>
  <subscription Id="Base" RigidName="ServerProtectionLinux-Base" Tag="RECOMMENDED" BaseVersion="10" FixedVersion="11"/>
</cloud_subscriptions>
    <delay_supplements enabled="true"/>
  </AUConfig>
  <Features>
  </Features>
</AUConfigurations>
)sophos" ;

    UpdatePolicyTranslator translator;
    auto settingsHolder = translator.translatePolicy(policy);
    auto config = settingsHolder.configurationData;

    EXPECT_TRUE(config.getUseSlowSupplements());
}


TEST_F(TestUpdatePolicyTranslator, ParsePolicyWithNormalSupplements)
{

    const std::string policy = R"sophos(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
  <csc:Comp RevID="f6babe12a13a5b2134c5861d01aed0eaddc20ea374e3a717ee1ea1451f5e2cf6" policyType="1"/>
  <AUConfig platform="Linux">
<cloud_subscriptions>
  <subscription Id="Base" RigidName="ServerProtectionLinux-Base" Tag="RECOMMENDED" BaseVersion="10" FixedVersion="11"/>
</cloud_subscriptions>
    <delay_supplements enabled="false"/>
  </AUConfig>
  <Features>
  </Features>
</AUConfigurations>
)sophos" ;

    UpdatePolicyTranslator translator;
    auto settingsHolder = translator.translatePolicy(policy);
    auto config = settingsHolder.configurationData;

    EXPECT_FALSE(config.getUseSlowSupplements());
}


TEST_F(TestUpdatePolicyTranslator, TelemetryAndUpdatePolicyAreSafeToBeAcquiredConcurrently)
{
    UpdatePolicyTranslator translator;
    std::ignore = translator.translatePolicy(mdrSSPLBasePolicy);
    Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
    auto thread1 = std::async(std::launch::async, []() {
          const std::string expectedTelemetry{
              R"sophos({"esmName":"","esmToken":"","subscriptions-ServerProtectionLinux-Base":"RECOMMENDED","subscriptions-ServerProtectionLinux-Plugin-MDR":"RECOMMENDED"})sophos"
          };
          for (int i = 0; i < 1000; i++)
          {
              std::string telemetry = Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
              EXPECT_EQ(telemetry, expectedTelemetry) << "Iteration: " << i;
          }
      });
    auto thread2 = std::async(std::launch::async, [&translator]() {
          for (int i = 0; i < 10; i++)
          {
              std::ignore = translator.translatePolicy(mdrSSPLBasePolicy);
          }
      });
    thread1.get();
    thread2.get();
}

using namespace UpdateSchedulerImpl;

class TestUpdatePolicyTelemetry : public ::testing::Test
{
public:
    void SetUp()  override
    {
        Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
    }
};

TEST_F(TestUpdatePolicyTelemetry, no_esm_no_fixedversion)
{
    UpdatePolicyTelemetry updatePolicyTelemetry;

    ESMVersion esmVersion;
    ProductSubscription productSubscription("rigidname", "baseversion", "tag", "");
    UpdatePolicyTelemetry::SubscriptionVector subscriptionVector {productSubscription};

    updatePolicyTelemetry.updateSubscriptions(subscriptionVector, esmVersion);
    updatePolicyTelemetry.setSubscriptions(Common::Telemetry::TelemetryHelper::getInstance());

    auto telemetry = Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
    EXPECT_EQ(telemetry,    R"({"esmName":"","esmToken":"","subscriptions-rigidname":"tag"})");
}

TEST_F(TestUpdatePolicyTelemetry, no_esm_fixedVersion)
{
    UpdatePolicyTelemetry updatePolicyTelemetry;

    ESMVersion esmVersion;
    ProductSubscription productSubscription("rigidname", "baseversion", "tag", "fixedversion");
    UpdatePolicyTelemetry::SubscriptionVector subscriptionVector {productSubscription};

    updatePolicyTelemetry.updateSubscriptions(subscriptionVector, esmVersion);
    updatePolicyTelemetry.setSubscriptions(Common::Telemetry::TelemetryHelper::getInstance());

    auto telemetry = Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
    EXPECT_EQ(telemetry,    R"({"esmName":"","esmToken":"","subscriptions-rigidname":"fixedversion"})");
}

TEST_F(TestUpdatePolicyTelemetry, esm_no_fixedVersion)
{
    UpdatePolicyTelemetry updatePolicyTelemetry;

    ESMVersion esmVersion("esmname", "esmtoken");
    ProductSubscription productSubscription("rigidname", "baseversion", "tag", "");
    UpdatePolicyTelemetry::SubscriptionVector subscriptionVector {productSubscription};

    updatePolicyTelemetry.updateSubscriptions(subscriptionVector, esmVersion);
    updatePolicyTelemetry.setSubscriptions(Common::Telemetry::TelemetryHelper::getInstance());

    auto telemetry = Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
    EXPECT_EQ(telemetry,    R"({"esmName":"esmname","esmToken":"esmtoken","subscriptions-rigidname":"esmname"})");
}

TEST_F(TestUpdatePolicyTelemetry, esm_fixedversion)
{
    UpdatePolicyTelemetry updatePolicyTelemetry;

    ESMVersion esmVersion("esmname", "esmtoken");
    ProductSubscription productSubscription("rigidname", "baseversion", "tag", "fixedversion");
    UpdatePolicyTelemetry::SubscriptionVector subscriptionVector {productSubscription};

    updatePolicyTelemetry.updateSubscriptions(subscriptionVector, esmVersion);
    updatePolicyTelemetry.setSubscriptions(Common::Telemetry::TelemetryHelper::getInstance());

    auto telemetry = Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
    EXPECT_EQ(telemetry,    R"({"esmName":"esmname","esmToken":"esmtoken","subscriptions-rigidname":"esmname"})");
}

TEST_F(TestUpdatePolicyTelemetry, esm_fixedversion_multiple)
{
    UpdatePolicyTelemetry updatePolicyTelemetry;

    ESMVersion esmVersion("esmname", "esmtoken");
    ProductSubscription productSubscription("rigidname", "baseversion", "tag", "fixedversion");
    ProductSubscription productSubscription2("rigidname2", "baseversion2", "tag2", "fixedversion2");
    UpdatePolicyTelemetry::SubscriptionVector subscriptionVector {productSubscription, productSubscription2};

    updatePolicyTelemetry.updateSubscriptions(subscriptionVector, esmVersion);
    updatePolicyTelemetry.setSubscriptions(Common::Telemetry::TelemetryHelper::getInstance());

    auto telemetry = Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
    EXPECT_EQ(telemetry,    R"({"esmName":"esmname","esmToken":"esmtoken","subscriptions-rigidname":"esmname","subscriptions-rigidname2":"esmname"})");
}

TEST_F(TestUpdatePolicyTelemetry, invalid_esm_no_fixedVersion)
{
    UpdatePolicyTelemetry updatePolicyTelemetry;

    ESMVersion esmVersion("esmname", "");
    ProductSubscription productSubscription("rigidname", "baseversion", "tag", "");
    UpdatePolicyTelemetry::SubscriptionVector subscriptionVector {productSubscription};

    updatePolicyTelemetry.updateSubscriptions(subscriptionVector, esmVersion);
    updatePolicyTelemetry.setSubscriptions(Common::Telemetry::TelemetryHelper::getInstance());

    auto telemetry = Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
    EXPECT_EQ(telemetry,    R"({"esmName":"","esmToken":"","subscriptions-rigidname":"tag"})");
}