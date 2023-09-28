// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "ThreatDetected.capnp.h"

#include "pluginimpl/StringUtils.h"
#include "scan_messages/SampleThreatDetected.h"
#include "tests/common/LogInitializedTests.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"

#include <capnp/serialize.h>
#include <gtest/gtest.h>
#include "scan_messages/ThreatDetected.h"

using namespace pluginimpl;
using namespace scan_messages;
using namespace common::CentralEnums;

namespace
{
    class TestStringUtils : public LogInitializedTests
    {
    };
} // namespace

TEST_F(TestStringUtils, TestgenerateThreatDetectedXml)
{
    std::string dataAsString = createThreatDetected({}).serialise();

    const kj::ArrayPtr<const capnp::word> view(
        reinterpret_cast<const capnp::word*>(&(*std::begin(dataAsString))),
        reinterpret_cast<const capnp::word*>(&(*std::end(dataAsString))));

    capnp::FlatArrayMessageReader messageInput(view);
    Sophos::ssplav::ThreatDetected::Reader deSerialisedData = messageInput.getRoot<Sophos::ssplav::ThreatDetected>();

    scan_messages::ThreatDetected threatDetectedMessage = scan_messages::ThreatDetected::deserialise(deSerialisedData);

    std::string result = generateThreatDetectedXml(threatDetectedMessage);

    const std::string expectedXML = R"sophos(<?xml version="1.0" encoding="utf-8"?>
<event type="sophos.core.detection" ts="1970-01-01T00:02:03.000Z">
  <user userId="username"/>
  <alert id="fedcba98-7654-3210-fedc-ba9876543210" name="EICAR-AV-Test" threatType="1" origin="1" remote="true">
    <sha256>131f95c51cc819465fa1797f6ccacf9d494aaaff46fa3eac73ae63ffbdfd8267</sha256>
    <path>/path/to/eicar.txt</path>
  </alert>
</event>)sophos";

    EXPECT_EQ(result, expectedXML);
}

TEST_F(TestStringUtils, TestgenerateThreatDetectedXmlUmlats)
{
    scan_messages::ThreatDetected threatDetected = createThreatDetected({
        .userID = "πολλῶν δ’ ἀνθρώπων ἴδεν ἄστεα καὶ νόον ἔγνω,, German umlats: Ä Ö Ü ß",
        .threatName = "Ἄνδρα μοι ἔννεπε, Μοῦσα, πολύτροπον, ὃς μάλα πολλὰ",
        .filePath = "/πλάγχθη, ἐπεὶ Τροίης ἱερὸν πτολίεθρον ἔπερσε·",
    });

    std::string dataAsString = threatDetected.serialise();

    const kj::ArrayPtr<const capnp::word> view(
        reinterpret_cast<const capnp::word*>(&(*std::begin(dataAsString))),
        reinterpret_cast<const capnp::word*>(&(*std::end(dataAsString))));

    capnp::FlatArrayMessageReader messageInput(view);
    Sophos::ssplav::ThreatDetected::Reader deSerialisedData = messageInput.getRoot<Sophos::ssplav::ThreatDetected>();

    scan_messages::ThreatDetected threatDetectedMessage = scan_messages::ThreatDetected::deserialise(deSerialisedData);
    std::string result = generateThreatDetectedXml(threatDetectedMessage);

    const std::string expectedXML = R"sophos(<?xml version="1.0" encoding="utf-8"?>
<event type="sophos.core.detection" ts="1970-01-01T00:02:03.000Z">
  <user userId="πολλῶν δ’ ἀνθρώπων ἴδεν ἄστεα καὶ νόον ἔγνω,, German umlats: Ä Ö Ü ß"/>
  <alert id="fedcba98-7654-3210-fedc-ba9876543210" name="Ἄνδρα μοι ἔννεπε, Μοῦσα, πολύτροπον, ὃς μάλα πολλὰ" threatType="1" origin="1" remote="true">
    <sha256>131f95c51cc819465fa1797f6ccacf9d494aaaff46fa3eac73ae63ffbdfd8267</sha256>
    <path>/πλάγχθη, ἐπεὶ Τροίης ἱερὸν πτολίεθρον ἔπερσε·</path>
  </alert>
</event>)sophos";

    EXPECT_EQ(result, expectedXML);
}

TEST_F(TestStringUtils, TestgenerateThreatDetectedXmlJapaneseCharacters)
{
    scan_messages::ThreatDetected threatDetected = createThreatDetected({
        .userID = "羅針盤なんて 渋滞のもと",
        .threatName = "ありったけの夢をかき集め",
        .filePath = "/捜し物を探しに行くのさ ONE PIECE",
    });

    std::string dataAsString = threatDetected.serialise();

    const kj::ArrayPtr<const capnp::word> view(
        reinterpret_cast<const capnp::word*>(&(*std::begin(dataAsString))),
        reinterpret_cast<const capnp::word*>(&(*std::end(dataAsString))));

    capnp::FlatArrayMessageReader messageInput(view);
    Sophos::ssplav::ThreatDetected::Reader deSerialisedData = messageInput.getRoot<Sophos::ssplav::ThreatDetected>();

    scan_messages::ThreatDetected threatDetectedMessage = scan_messages::ThreatDetected::deserialise(deSerialisedData);
    std::string result = generateThreatDetectedXml(threatDetectedMessage);

    const std::string expectedXML = R"sophos(<?xml version="1.0" encoding="utf-8"?>
<event type="sophos.core.detection" ts="1970-01-01T00:02:03.000Z">
  <user userId="羅針盤なんて 渋滞のもと"/>
  <alert id="fedcba98-7654-3210-fedc-ba9876543210" name="ありったけの夢をかき集め" threatType="1" origin="1" remote="true">
    <sha256>131f95c51cc819465fa1797f6ccacf9d494aaaff46fa3eac73ae63ffbdfd8267</sha256>
    <path>/捜し物を探しに行くのさ ONE PIECE</path>
  </alert>
</event>)sophos";

    EXPECT_EQ(result, expectedXML);
}

TEST_F(TestStringUtils, TestEmptyPathXML)
{
    scan_messages::ThreatDetected threatDetectedMessage = createThreatDetected({ .filePath = "" });
    std::string result = generateThreatDetectedXml(threatDetectedMessage);

    const std::string expectedXML = R"sophos(<?xml version="1.0" encoding="utf-8"?>
<event type="sophos.core.detection" ts="1970-01-01T00:02:03.000Z">
  <user userId="username"/>
  <alert id="fedcba98-7654-3210-fedc-ba9876543210" name="EICAR-AV-Test" threatType="1" origin="1" remote="true">
    <sha256>131f95c51cc819465fa1797f6ccacf9d494aaaff46fa3eac73ae63ffbdfd8267</sha256>
    <path></path>
  </alert>
</event>)sophos";

    EXPECT_EQ(result, expectedXML);
}

TEST_F(TestStringUtils, TestLongPathXML)
{
    std::string longPath(centralLimitedStringMaxSize + 1, '/');
    std::string truncatedPath(centralLimitedStringMaxSize, '/');

    scan_messages::ThreatDetected threatDetectedMessage = createThreatDetected({ .filePath = longPath });
    std::string result = generateThreatDetectedXml(threatDetectedMessage);

    const std::string expectedXML = R"sophos(<?xml version="1.0" encoding="utf-8"?>
<event type="sophos.core.detection" ts="1970-01-01T00:02:03.000Z">
  <user userId="username"/>
  <alert id="fedcba98-7654-3210-fedc-ba9876543210" name="EICAR-AV-Test" threatType="1" origin="1" remote="true">
    <sha256>131f95c51cc819465fa1797f6ccacf9d494aaaff46fa3eac73ae63ffbdfd8267</sha256>
    <path>)sophos" + truncatedPath + R"sophos(</path>
  </alert>
</event>)sophos";

    EXPECT_EQ(result, expectedXML);
}

TEST_F(TestStringUtils, TestEmptyThreatPathJSON)
{
    std::string result = generateThreatDetectedJson(createThreatDetected({ .filePath = "" }));

    static const std::string expectedJSON =
        R"sophos({"avScanType":203,"details":{"filePath":"","sha256FileHash":"131f95c51cc819465fa1797f6ccacf9d494aaaff46fa3eac73ae63ffbdfd8267"},"detectionName":{"short":"EICAR-AV-Test"},"items":{"1":{"path":"","primary":true,"sha256":"131f95c51cc819465fa1797f6ccacf9d494aaaff46fa3eac73ae63ffbdfd8267","type":1}},"quarantineSuccess":false,"threatSource":1,"threatType":1,"time":123})sophos";

    EXPECT_EQ(result, expectedJSON);
}

TEST_F(TestStringUtils, TestOnDemandEventJson)
{
    // Set pid and executablePath to prove they are not included in the JSON even if set
    std::string result = generateThreatDetectedJson(createThreatDetected({ .pid = 1, .executablePath = "path" }));

    const std::string expectedJSON =
        R"sophos({"avScanType":203,"details":{"filePath":"/path/to/eicar.txt","sha256FileHash":"131f95c51cc819465fa1797f6ccacf9d494aaaff46fa3eac73ae63ffbdfd8267"},"detectionName":{"short":"EICAR-AV-Test"},"items":{"1":{"path":"/path/to/eicar.txt","primary":true,"sha256":"131f95c51cc819465fa1797f6ccacf9d494aaaff46fa3eac73ae63ffbdfd8267","type":1}},"quarantineSuccess":false,"threatSource":1,"threatType":1,"time":123})sophos";

    EXPECT_EQ(result, expectedJSON);
}

TEST_F(TestStringUtils, TestOnAccessEventJson)
{
    std::string result = generateThreatDetectedJson(createOnAccessThreatDetected({}));

    const std::string expectedJSON =
        R"sophos({"avScanType":201,"details":{"filePath":"/path/to/eicar.txt","sha256FileHash":"131f95c51cc819465fa1797f6ccacf9d494aaaff46fa3eac73ae63ffbdfd8267"},"detectionName":{"short":"EICAR-AV-Test"},"items":{"1":{"path":"/path/to/eicar.txt","primary":true,"sha256":"131f95c51cc819465fa1797f6ccacf9d494aaaff46fa3eac73ae63ffbdfd8267","type":1}},"pid":100,"processPath":"/path/to/executable","quarantineSuccess":false,"threatSource":1,"threatType":1,"time":123})sophos";

    EXPECT_EQ(result, expectedJSON);
}

TEST_F(TestStringUtils, TestEmptyThreatNameJSON)
{
    std::string result = generateThreatDetectedJson(createThreatDetected({ .threatName = "" }));

    const std::string expectedJSON =
        R"sophos({"avScanType":203,"details":{"filePath":"/path/to/eicar.txt","sha256FileHash":"131f95c51cc819465fa1797f6ccacf9d494aaaff46fa3eac73ae63ffbdfd8267"},"detectionName":{"short":""},"items":{"1":{"path":"/path/to/eicar.txt","primary":true,"sha256":"131f95c51cc819465fa1797f6ccacf9d494aaaff46fa3eac73ae63ffbdfd8267","type":1}},"quarantineSuccess":false,"threatSource":1,"threatType":1,"time":123})sophos";

    EXPECT_EQ(result, expectedJSON);
}

TEST_F(TestStringUtils, TestEmptyThreatTypeJSON)
{
    std::string result = generateThreatDetectedJson(createThreatDetected({ .threatType = "" }));

    const std::string expectedJSON =
        R"sophos({"avScanType":203,"details":{"filePath":"/path/to/eicar.txt","sha256FileHash":"131f95c51cc819465fa1797f6ccacf9d494aaaff46fa3eac73ae63ffbdfd8267"},"detectionName":{"short":"EICAR-AV-Test"},"items":{"1":{"path":"/path/to/eicar.txt","primary":true,"sha256":"131f95c51cc819465fa1797f6ccacf9d494aaaff46fa3eac73ae63ffbdfd8267","type":1}},"quarantineSuccess":false,"threatSource":1,"threatType":0,"time":123})sophos";

    EXPECT_EQ(result, expectedJSON);
}


TEST_F(TestStringUtils, TestGenerateOnAcessConfig)
{
    std::vector<std::string> exclusionList = { "x", "y", "z" };

    std::string expectedResult = R"({"enabled":true,"excludeRemoteFiles":true,"exclusions":["x","y","z"]})";
    EXPECT_EQ(expectedResult, generateOnAccessConfig(true, exclusionList, true));

    expectedResult = R"({"enabled":false,"excludeRemoteFiles":false,"exclusions":["x","y","z"]})";
    EXPECT_EQ(expectedResult, generateOnAccessConfig(false, exclusionList, false));

    exclusionList = {};
    expectedResult = R"({"enabled":false,"excludeRemoteFiles":true,"exclusions":[]})";
    EXPECT_EQ(expectedResult, generateOnAccessConfig(false, exclusionList, true));
}

TEST_F(TestStringUtils, generateCoreCleanEventXmlQuarantineSuccess)
{
    std::string expectedEventXml = R"(<?xml version="1.0" encoding="utf-8"?>
<event type="sophos.core.clean" ts="1970-01-01T00:02:03.000Z">
  <alert id="fedcba98-7654-3210-fedc-ba9876543210" succeeded="1" origin="1">
    <items totalItems="1">
      <item type="file" result="0">
        <descriptor>/path/to/eicar.txt</descriptor>
      </item>
    </items>
  </alert>
</event>)";

    std::string xml;
    ASSERT_NO_THROW(
        xml = generateCoreCleanEventXml(
            createThreatDetected({ .quarantineResult = common::CentralEnums::QuarantineResult::SUCCESS })));
    ASSERT_EQ(xml, expectedEventXml);
}

TEST_F(TestStringUtils, generateCoreCleanEventXmlQuarantineFailedToDeleteFile)
{
    std::string expectedEventXml = R"(<?xml version="1.0" encoding="utf-8"?>
<event type="sophos.core.clean" ts="1970-01-01T00:02:03.000Z">
  <alert id="fedcba98-7654-3210-fedc-ba9876543210" succeeded="0" origin="1">
    <items totalItems="1">
      <item type="file" result="3">
        <descriptor>/path/to/eicar.txt</descriptor>
      </item>
    </items>
  </alert>
</event>)";

    std::string xml;
    ASSERT_NO_THROW(
        xml = generateCoreCleanEventXml(createThreatDetected(
            { .quarantineResult = common::CentralEnums::QuarantineResult::FAILED_TO_DELETE_FILE })));
    ASSERT_EQ(xml, expectedEventXml);
}

TEST_F(TestStringUtils, generateCoreCleanEventXmlFromMlDetection)
{
    std::string expectedEventXml = R"(<?xml version="1.0" encoding="utf-8"?>
<event type="sophos.core.clean" ts="1970-01-01T00:02:03.000Z">
  <alert id="fedcba98-7654-3210-fedc-ba9876543210" succeeded="0" origin="0">
    <items totalItems="1">
      <item type="file" result="3">
        <descriptor>/path/to/eicar.txt</descriptor>
      </item>
    </items>
  </alert>
</event>)";

    std::string xml;
    ASSERT_NO_THROW(
        xml = generateCoreCleanEventXml(
            createThreatDetected({ .quarantineResult = common::CentralEnums::QuarantineResult::FAILED_TO_DELETE_FILE,
                                   .reportSource = ReportSource::ml })));
    ASSERT_EQ(xml, expectedEventXml);
}

TEST_F(TestStringUtils, generateCoreRestoreEventXml)
{
    scan_messages::RestoreReport restoreReport{ 123, "/threat/path", "fedcba98-7654-3210-fedc-ba9876543210", false };

    std::string expectedEventXml = R"(<?xml version="1.0" encoding="utf-8"?>
<event type="sophos.core.restore" ts="1970-01-01T00:02:03.000Z">
  <alert id="fedcba98-7654-3210-fedc-ba9876543210" succeeded="0">
    <items totalItems="1">
      <item type="file">
        <descriptor>/threat/path</descriptor>
      </item>
    </items>
  </alert>
</event>)";

    std::string xml;
    ASSERT_NO_THROW(xml = generateCoreRestoreEventXml(restoreReport));
    ASSERT_EQ(xml, expectedEventXml);
}

TEST_F(TestStringUtils, TestBadUnicodePaths)
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    appConfig.setData("PLUGIN_INSTALL", "/test/");

    const std::string expectedXML = R"sophos(<?xml version="1.0" encoding="utf-8"?>
<event type="sophos.core.detection" ts="1970-01-01T00:02:03.000Z">
  <user userId="username"/>
  <alert id="fedcba98-7654-3210-fedc-ba9876543210" name="EICAR-AV-Test" threatType="1" origin="1" remote="true">
    <sha256>131f95c51cc819465fa1797f6ccacf9d494aaaff46fa3eac73ae63ffbdfd8267</sha256>
    <path>See endpoint logs for threat file path at: /test/log/sophos_threat_detector/sophos_threat_detector.log</path>
  </alert>
</event>)sophos";

    EXPECT_EQ(generateThreatDetectedXml(createThreatDetected({ .filePath = "\xef\xbf\xbe" })), expectedXML);
    EXPECT_EQ(generateThreatDetectedXml(createThreatDetected({ .filePath = "\xef\xbf\xbf" })), expectedXML);
    EXPECT_EQ(generateThreatDetectedXml(createThreatDetected({ .filePath = "\xef\xbf\xbe\xef\xbf\xbf" })), expectedXML);
}