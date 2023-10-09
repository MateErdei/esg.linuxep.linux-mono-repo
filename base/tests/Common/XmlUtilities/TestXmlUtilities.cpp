/******************************************************************************************************

Copyright 2018-2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/XmlUtilities/AttributesMap.h>
#include <Common/XmlUtilities/Validation.h>
#include <gtest/gtest.h>
#include <include/gmock/gmock-matchers.h>

using namespace Common::XmlUtilities;

// NOLINTNEXTLINE(cert-err58-cpp)
static std::string updatePolicy{ R"sophos(<?xml version="1.0"?>
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
    <schedule AllowLocalConfig="false" SchedEnable="true" Frequency="60" DetectDialUp="false"/>

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
      <location hostname="2k12-64-ld55-df.eng.sophos:8191" priority="0" id="4092822d-0925-4deb-9146-fbc8532f8c55"/>
      <location hostname="maineng2.eng.sophos:8191" priority="0" id="d4739ed5-b66a-4a21-9218-4fd0dfe4dcbd"/>
      <location hostname="w2k8r2-std-en-df.eng.sophos:8191" priority="0" id="0e1e598b-d6d6-4e4b-9425-9fefd277a353"/>
    </locations>
  </update_cache>
  <customer id="4b4ca3ba-c144-4447-8050-6c96a7104c11"/>
</AUConfigurations>
)sophos" };

// NOLINTNEXTLINE(cert-err58-cpp)
static std::string ENTITY_XML{ R"sophos(<!DOCTYPE xmlbomb [
<!ENTITY a "1234567890" >
<!ENTITY b "&a;&a;&a;&a;&a;&a;&a;&a;">
<!ENTITY c "&b;&b;&b;&b;&b;&b;&b;&b;">
]>
<bomb>&c;</bomb>
)sophos" };

TEST(TestXmlUtilities, ParsePrimaryLocationUsername) // NOLINT
{
    auto simpleXml = parseXml(updatePolicy);

    ASSERT_EQ(simpleXml.lookup("AUConfigurations/AUConfig/primary_location/server").value("UserName"), "W2YJXI6FED");
}

TEST(TestXmlUtilities, ConcatenateAttributesIdAndHandleText) // NOLINT
{
    auto simpleXml = parseXml(updatePolicy);
    auto attributes_fullname = simpleXml.entitiesThatContainPath(
        "AUConfigurations/update_cache/intermediate_certificates/intermediate_certificate");
    ASSERT_EQ(attributes_fullname.size(), 3);
    std::string one_full_path = "AUConfigurations/update_cache/intermediate_certificates/"
                                "intermediate_certificate#33d6a435957397fc9336c8633445aa33e1774500.crt";

    ASSERT_TRUE(
        std::find(std::begin(attributes_fullname), std::end(attributes_fullname), one_full_path) !=
        std::end(attributes_fullname));
    auto attributes = simpleXml.lookup(one_full_path);

    ASSERT_THAT(attributes.value(attributes.TextId), ::testing::HasSubstr("-----BEGIN CERTIFICATE-----"));
    ASSERT_THAT(attributes.value(attributes.TextId), ::testing::HasSubstr("-----END CERTIFICATE-----"));
}

TEST(TestXmlUtilities, EntitiesAreRejected) // NOLINT
{
    Common::Logging::ConsoleLoggingSetup loggingSetup;
    try
    {
        (void)parseXml(ENTITY_XML);
        FAIL() << "Able to parse Entity XML";
    }
    catch (const XmlUtilitiesException& ex)
    {
        // All good
    }
}

TEST(TestXmlUtilities, DuplicateElementsCanBeRetrieved) // NOLINT
{
    auto simpleXml = parseXml("<xml><a>ONE</a><a>TWO</a></xml>");
    auto attributesList = simpleXml.lookupMultiple("xml/a");
    ASSERT_EQ(attributesList.size(), 2);
    EXPECT_EQ(attributesList[0].contents(), "ONE");
    EXPECT_EQ(attributesList[1].contents(), "TWO");

    // Test that the right names are present
    auto entityNames = simpleXml.entitiesThatContainPath("xml/a");
    EXPECT_EQ(entityNames[0], "xml/a");
    EXPECT_EQ(entityNames[1], "xml/a_0");
}

TEST(TestXmlUtilities, DuplicateElementsWithIdCanBeRetrieved) // NOLINT
{
    auto simpleXml = parseXml(R"(<xml><a id="x">ONE</a><a id="x">TWO</a></xml>)");
    auto attributesList = simpleXml.lookupMultiple("xml/a");
    ASSERT_EQ(attributesList.size(), 2);
    EXPECT_EQ(attributesList[0].contents(), "ONE");
    EXPECT_EQ(attributesList[1].contents(), "TWO");

    auto entityNames = simpleXml.entitiesThatContainPath("xml/a");
    EXPECT_EQ(entityNames[0], "xml/a#x");
    EXPECT_EQ(entityNames[1], "xml/a#x_0");
}


TEST(TestXmlUtilities, DuplicateElementsWithIdsCanBeRetrieved) // NOLINT
{
    auto simpleXml = parseXml(R"(<xml><a id="1">ONE</a><a id="2">TWO</a></xml>)");
    auto attributesList = simpleXml.lookupMultiple("xml/a");
    ASSERT_EQ(attributesList.size(), 2);
    EXPECT_EQ(attributesList[0].value("id"), "1");
    EXPECT_EQ(attributesList[1].value("id"), "2");
}

TEST(TestXmlUtilities, ChildElementsAreNotRetrieved) // NOLINT
{
    auto simpleXml = parseXml(R"(<xml><a id="1">ONE<a id="99"/></a><a id="2">TWO<a id="98"/></a></xml>)");
    auto attributesList = simpleXml.lookupMultiple("xml/a");
    ASSERT_EQ(attributesList.size(), 2);
    EXPECT_EQ(attributesList[0].value("id"), "1");
    EXPECT_EQ(attributesList[0].contents(), "ONE");
    EXPECT_EQ(attributesList[1].value("id"), "2");
    EXPECT_EQ(attributesList[1].contents(), "TWO");
}

TEST(TestXmlUtilities, GetIdFromElement) // NOLINT
{
    /*
     * This is an investigation around the bug LINUXDAR-735
     * entitiesThatContainPath will return child elements of the queried elements.
     * lookupMultiple doesn't return child elements
     * Adding the # prevents either from returning prefix-extending versions
     */

    std::string policySnippet = R"(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
  <customer id="a">
    <entry id="b"/>
  </customer>
  <customerflag id="c"/>
</AUConfigurations>)";

    auto simpleXml = parseXml(policySnippet);
    auto keys = simpleXml.entitiesThatContainPath("AUConfigurations/customer#");
    ASSERT_EQ(keys.size(), 2);
    EXPECT_EQ(keys[0], "AUConfigurations/customer#a");
    auto attrs = simpleXml.lookup(keys[0]);
    EXPECT_EQ(attrs.value("id"), "a");

    keys = simpleXml.entitiesThatContainPath("AUConfigurations/customer#", false);
    ASSERT_EQ(keys.size(), 1);
    EXPECT_EQ(keys[0], "AUConfigurations/customer#a");
    attrs = simpleXml.lookup(keys[0]);
    EXPECT_EQ(attrs.value("id"), "a");

    auto attributesList = simpleXml.lookupMultiple("AUConfigurations/customer#");
    ASSERT_EQ(attributesList.size(), 1);
    EXPECT_EQ(attributesList[0].value("id"), "a");
}

TEST(TestXmlUtilities, DuplicateStructuresAreAvailable) // NOLINT
{
    std::string policySnippet = R"MULTILINE(<?xml version="1.0"?>
<config>
    <scanSet>
      <!-- if {{scheduledScanEnabled}} -->
      <scan>
        <name>Sophos Cloud Scheduled Scan</name>
        <schedule>
          <daySet>
            <!-- for day in {{scheduledScanDays}} -->
            <day>{{day}}</day>
          </daySet>
          <timeSet>
            <time>{{scheduledScanTime}}</time>
          </timeSet>
        </schedule>
        <settings>
          <scanObjectSet>
            <CDDVDDrives>false</CDDVDDrives>
            <hardDrives>true</hardDrives>
            <networkDrives>false</networkDrives>
            <removableDrives>false</removableDrives>
            <kernelMemory>true</kernelMemory>
          </scanObjectSet>
          <scanBehaviour>
            <level>normal</level>
            <archives>{{scheduledScanArchives}}</archives>
            <pua>true</pua>
            <suspiciousFileDetection>false</suspiciousFileDetection>
            <scanForMacViruses>false</scanForMacViruses>
            <anti-rootkits>true</anti-rootkits>
          </scanBehaviour>
          <actions>
            <disinfect>true</disinfect>
            <puaRemoval>false</puaRemoval>
            <fileAction>doNothing</fileAction>
            <destination/>
            <suspiciousFiles>
              <fileAction>doNothing</fileAction>
              <destination/>
            </suspiciousFiles>
          </actions>
          <on-demand-options>
            <minimise-scan-impact>true</minimise-scan-impact>
          </on-demand-options>
        </settings>
      </scan>
      <scan>
        <name>Another scan!</name>
        <schedule>
          <daySet>
            <!-- for day in {{scheduledScanDays}} -->
            <day>Monday</day>
            <day>Tuesday</day>
            <day>Wednesday</day>
          </daySet>
          <timeSet>
            <time>{{scheduledScanTime}}</time>
          </timeSet>
        </schedule>
        <settings>
          <scanObjectSet>
            <CDDVDDrives>false</CDDVDDrives>
            <hardDrives>true</hardDrives>
            <networkDrives>false</networkDrives>
            <removableDrives>false</removableDrives>
            <kernelMemory>true</kernelMemory>
          </scanObjectSet>
          <scanBehaviour>
            <level>normal</level>
            <archives>{{scheduledScanArchives}}</archives>
            <pua>true</pua>
            <suspiciousFileDetection>false</suspiciousFileDetection>
            <scanForMacViruses>false</scanForMacViruses>
            <anti-rootkits>true</anti-rootkits>
          </scanBehaviour>
          <actions>
            <disinfect>true</disinfect>
            <puaRemoval>false</puaRemoval>
            <fileAction>doNothing</fileAction>
            <destination/>
            <suspiciousFiles>
              <fileAction>doNothing</fileAction>
              <destination/>
            </suspiciousFiles>
          </actions>
          <on-demand-options>
            <minimise-scan-impact>true</minimise-scan-impact>
          </on-demand-options>
        </settings>
      </scan>
    </scanSet>
</config>
)MULTILINE";

    auto simpleXml = parseXml(policySnippet);

    auto keys = simpleXml.entitiesThatContainPath("config/scanSet/scan", false);
    ASSERT_EQ(keys.size(), 2);
    EXPECT_EQ(keys[0], "config/scanSet/scan");
    EXPECT_EQ(keys[1], "config/scanSet/scan_0");

    auto key0 = keys[0];
    key0 += "/name";
    auto attr = simpleXml.lookup(key0);
    EXPECT_EQ(attr.contents(), "Sophos Cloud Scheduled Scan");

    auto key1 = keys[1];
    key1 += "/name";
    attr = simpleXml.lookup(key1);
    EXPECT_EQ(attr.contents(), "Another scan!");

    key1 = keys[1] + "/schedule/daySet/day";
    auto attrs = simpleXml.lookupMultiple(key1);
    ASSERT_EQ(attrs.size(), 3);
    EXPECT_EQ(attrs[0].contents(), "Monday");
    EXPECT_EQ(attrs[1].contents(), "Tuesday");
    EXPECT_EQ(attrs[2].contents(), "Wednesday");
}

TEST(TestXmlUtilities, ValidXmlString) // NOLINT
{
    std::string xmlString = "dummy xml string";
    bool result = Validation::stringWillNotBreakXmlParsing(xmlString);

    ASSERT_EQ(result, true);
}

TEST(TestXmlUtilities, InvalidXmlStrings) // NOLINT
{
    std::string xmlStringWithInvalidSymbols = "<fakeXmlInjection>dummy xml string</fakeXmlInjection>";
    std::string xmlStringWithAmpersand = "dummy& xml string";
    std::string xmlStringWithSingleQuote = "\'dummy xml string\'";
    std::string xmlStringWithDoubleQuote = "'dummy \"xml\" string'";

    ASSERT_EQ(Validation::stringWillNotBreakXmlParsing(xmlStringWithInvalidSymbols), false);
    ASSERT_EQ(Validation::stringWillNotBreakXmlParsing(xmlStringWithAmpersand), false);
    ASSERT_EQ(Validation::stringWillNotBreakXmlParsing(xmlStringWithSingleQuote), false);
    ASSERT_EQ(Validation::stringWillNotBreakXmlParsing(xmlStringWithDoubleQuote), false);
}