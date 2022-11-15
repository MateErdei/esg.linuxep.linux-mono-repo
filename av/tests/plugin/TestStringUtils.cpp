// Copyright 2020-2022, Sophos Limited. All rights reserved.

#include "ThreatDetected.capnp.h"

#include "pluginimpl/StringUtils.h"
#include "tests/common/LogInitializedTests.h"

#include <capnp/serialize.h>
#include <gtest/gtest.h>
#include <scan_messages/ThreatDetected.h>

using namespace pluginimpl;
using namespace scan_messages;
using namespace common::CentralEnums;

namespace
{

    std::time_t DUMMY_TIMESTAMP = 123;

    class TestStringUtils : public LogInitializedTests
    {
    public:
        std::string m_sha256 = "590165d5fd84b3302e239c260867a2310af2bc3b519b5c9c68ab2515c9bad15b";
        std::string m_threatId = "c1c802c6-a878-ee05-babc-c0378d45d8d4";
        std::string m_englishsXML = R"sophos(<?xml version="1.0" encoding="utf-8"?>
<event type="sophos.core.detection" ts="1970-01-01T00:02:03.000Z">
  <user userId="User"/>
  <alert id="c1c802c6-a878-ee05-babc-c0378d45d8d4" name="eicar" threatType="1" origin="0" remote="false">
    <sha256>590165d5fd84b3302e239c260867a2310af2bc3b519b5c9c68ab2515c9bad15b</sha256>
    <path>path/to/threat</path>
  </alert>
</event>)sophos";

        std::string m_umlatsXML = R"sophos(<?xml version="1.0" encoding="utf-8"?>
<event type="sophos.core.detection" ts="1970-01-01T00:02:03.000Z">
  <user userId="πολλῶν δ’ ἀνθρώπων ἴδεν ἄστεα καὶ νόον ἔγνω,, German umlats: Ä Ö Ü ß"/>
  <alert id="c1c802c6-a878-ee05-babc-c0378d45d8d4" name="Ἄνδρα μοι ἔννεπε, Μοῦσα, πολύτροπον, ὃς μάλα πολλὰ" threatType="1" origin="0" remote="false">
    <sha256>590165d5fd84b3302e239c260867a2310af2bc3b519b5c9c68ab2515c9bad15b</sha256>
    <path>/πλάγχθη, ἐπεὶ Τροίης ἱερὸν πτολίεθρον ἔπερσε·</path>
  </alert>
</event>)sophos";

        std::string m_japaneseXML = R"sophos(<?xml version="1.0" encoding="utf-8"?>
<event type="sophos.core.detection" ts="1970-01-01T00:02:03.000Z">
  <user userId="羅針盤なんて 渋滞のもと"/>
  <alert id="c1c802c6-a878-ee05-babc-c0378d45d8d4" name="ありったけの夢をかき集め" threatType="1" origin="0" remote="false">
    <sha256>590165d5fd84b3302e239c260867a2310af2bc3b519b5c9c68ab2515c9bad15b</sha256>
    <path>/捜し物を探しに行くのさ ONE PIECE</path>
  </alert>
</event>)sophos";

    scan_messages::ThreatDetected m_simpleThreatDetected =  scan_messages::ThreatDetected (
        "User ID",
        DUMMY_TIMESTAMP,
        ThreatType::virus,
        "threat name",
        E_SCAN_TYPE_ON_ACCESS_OPEN,
        E_NOTIFICATION_STATUS_CLEANED_UP,
        "/threat/path",
        E_SMT_THREAT_ACTION_SHRED,
        "T2677b3f1607845d",
        m_threatId,
        false,
        ReportSource::ml,
        datatypes::AutoFd());
    };

}

TEST_F(TestStringUtils, TestgenerateThreatDetectedXml)
{
    std::string threatName = "eicar";
    std::string threatPath = "path/to/threat";
    std::string userID = "User";

    scan_messages::ThreatDetected threatDetected(
        userID,
        DUMMY_TIMESTAMP,
        ThreatType::virus,
        threatName,
        E_SCAN_TYPE_ON_ACCESS,
        E_NOTIFICATION_STATUS_CLEANED_UP,
        threatPath,
        E_SMT_THREAT_ACTION_SHRED,
        m_sha256,
        m_threatId,
        false,
        ReportSource::ml,
        datatypes::AutoFd());

    std::string dataAsString = threatDetected.serialise();

    const kj::ArrayPtr<const capnp::word> view(
            reinterpret_cast<const capnp::word*>(&(*std::begin(dataAsString))),
            reinterpret_cast<const capnp::word*>(&(*std::end(dataAsString))));

    capnp::FlatArrayMessageReader messageInput(view);
    Sophos::ssplav::ThreatDetected::Reader deSerialisedData =
            messageInput.getRoot<Sophos::ssplav::ThreatDetected>();

    scan_messages::ThreatDetected threatDetectedMessage(deSerialisedData);

    std::string result = generateThreatDetectedXml(threatDetectedMessage);

    EXPECT_EQ(result, m_englishsXML);
}

TEST_F(TestStringUtils, TestgenerateThreatDetectedXmlUmlats)
{
    std::string threatName = "Ἄνδρα μοι ἔννεπε, Μοῦσα, πολύτροπον, ὃς μάλα πολλὰ";
    std::string threatPath = "/πλάγχθη, ἐπεὶ Τροίης ἱερὸν πτολίεθρον ἔπερσε·";
    std::string userID = "πολλῶν δ’ ἀνθρώπων ἴδεν ἄστεα καὶ νόον ἔγνω,, German umlats: Ä Ö Ü ß";

    scan_messages::ThreatDetected threatDetected(
        userID,
        DUMMY_TIMESTAMP,
        ThreatType::virus,
        threatName,
        E_SCAN_TYPE_ON_ACCESS,
        E_NOTIFICATION_STATUS_CLEANED_UP,
        threatPath,
        E_SMT_THREAT_ACTION_SHRED,
        m_sha256,
        m_threatId,
        false,
        ReportSource::ml,
        datatypes::AutoFd());

    std::string dataAsString = threatDetected.serialise();

    const kj::ArrayPtr<const capnp::word> view(
            reinterpret_cast<const capnp::word*>(&(*std::begin(dataAsString))),
            reinterpret_cast<const capnp::word*>(&(*std::end(dataAsString))));

    capnp::FlatArrayMessageReader messageInput(view);
    Sophos::ssplav::ThreatDetected::Reader deSerialisedData =
            messageInput.getRoot<Sophos::ssplav::ThreatDetected>();

    scan_messages::ThreatDetected threatDetectedMessage(deSerialisedData);
    std::string result = generateThreatDetectedXml(threatDetectedMessage);

    EXPECT_EQ(result, m_umlatsXML);
}

TEST_F(TestStringUtils, TestgenerateThreatDetectedXmlJapaneseCharacters)
{
    std::string threatName = "ありったけの夢をかき集め";
    std::string threatPath = "/捜し物を探しに行くのさ ONE PIECE";
    std::string userID = "羅針盤なんて 渋滞のもと";

    scan_messages::ThreatDetected threatDetected(
        userID,
        DUMMY_TIMESTAMP,
        ThreatType::virus,
        threatName,
        E_SCAN_TYPE_ON_ACCESS,
        E_NOTIFICATION_STATUS_CLEANED_UP,
        threatPath,
        E_SMT_THREAT_ACTION_SHRED,
        m_sha256,
        m_threatId,
        false,
        ReportSource::ml,
        datatypes::AutoFd());

    std::string dataAsString = threatDetected.serialise();

    const kj::ArrayPtr<const capnp::word> view(
            reinterpret_cast<const capnp::word*>(&(*std::begin(dataAsString))),
            reinterpret_cast<const capnp::word*>(&(*std::end(dataAsString))));

    capnp::FlatArrayMessageReader messageInput(view);
    Sophos::ssplav::ThreatDetected::Reader deSerialisedData =
            messageInput.getRoot<Sophos::ssplav::ThreatDetected>();

    scan_messages::ThreatDetected threatDetectedMessage(deSerialisedData);
    std::string result = generateThreatDetectedXml(threatDetectedMessage);

    EXPECT_EQ(result, m_japaneseXML);
}

static scan_messages::ThreatDetected createEvent(
    const std::string& threatName = "",
    const std::string& threatPath = "",
    const std::string& userID = "",
    const std::string& sha256 = "")
{
    scan_messages::ThreatDetected threatDetected(
        userID,
        DUMMY_TIMESTAMP,
        ThreatType::virus,
        threatName,
        E_SCAN_TYPE_ON_ACCESS,
        E_NOTIFICATION_STATUS_CLEANED_UP,
        threatPath,
        E_SMT_THREAT_ACTION_SHRED,
        sha256,
        "00000000-0000-0000-0000-000000000000",
        false,
        ReportSource::ml,
        datatypes::AutoFd());

    std::string dataAsString = threatDetected.serialise();

    const kj::ArrayPtr<const capnp::word> view(
        reinterpret_cast<const capnp::word*>(&(*std::begin(dataAsString))),
        reinterpret_cast<const capnp::word*>(&(*std::end(dataAsString))));

    capnp::FlatArrayMessageReader messageInput(view);
    Sophos::ssplav::ThreatDetected::Reader deSerialisedData =
        messageInput.getRoot<Sophos::ssplav::ThreatDetected>();

    return scan_messages::ThreatDetected(deSerialisedData);
}

TEST_F(TestStringUtils, TestEmptyPathXML)
{
    scan_messages::ThreatDetected threatDetectedMessage(createEvent());
    std::string result = generateThreatDetectedXml(threatDetectedMessage);

    static const std::string expectedXML = R"sophos(<?xml version="1.0" encoding="utf-8"?>
<event type="sophos.core.detection" ts="1970-01-01T00:02:03.000Z">
  <user userId=""/>
  <alert id="00000000-0000-0000-0000-000000000000" name="" threatType="1" origin="0" remote="false">
    <sha256></sha256>
    <path></path>
  </alert>
</event>)sophos";

    EXPECT_EQ(result, expectedXML);
}

TEST_F(TestStringUtils, TestLongPathXML)
{
    std::string longPath(centralLimitedStringMaxSize + 1, '/');
    std::string truncatedPath(centralLimitedStringMaxSize, '/');

    scan_messages::ThreatDetected threatDetectedMessage(createEvent("", longPath));
    std::string result = generateThreatDetectedXml(threatDetectedMessage);

    static const std::string expectedXML = R"sophos(<?xml version="1.0" encoding="utf-8"?>
<event type="sophos.core.detection" ts="1970-01-01T00:02:03.000Z">
  <user userId=""/>
  <alert id="00000000-0000-0000-0000-000000000000" name="" threatType="1" origin="0" remote="false">
    <sha256></sha256>
    <path>)sophos" + truncatedPath + R"sophos(</path>
  </alert>
</event>)sophos";

    EXPECT_EQ(result, expectedXML);
}

TEST_F(TestStringUtils, TestEmptyThreatPathJSON)
{
    std::string threatName = "eicar";
    std::string userID = "User";

    scan_messages::ThreatDetected threatDetected(
        userID,
        DUMMY_TIMESTAMP,
        ThreatType::virus,
        threatName,
        E_SCAN_TYPE_ON_ACCESS_OPEN,
        E_NOTIFICATION_STATUS_CLEANED_UP,
        "",
        E_SMT_THREAT_ACTION_SHRED,
        "2677b3f1607845d18d5a405a8ef592e79b8a6de355a9b7490b6bb439c2116def",
        m_threatId,
        false,
        ReportSource::ml,
        datatypes::AutoFd());

    std::string dataAsString = threatDetected.serialise();

    const kj::ArrayPtr<const capnp::word> view(
        reinterpret_cast<const capnp::word*>(&(*std::begin(dataAsString))),
        reinterpret_cast<const capnp::word*>(&(*std::end(dataAsString))));

    capnp::FlatArrayMessageReader messageInput(view);
    Sophos::ssplav::ThreatDetected::Reader deSerialisedData =
        messageInput.getRoot<Sophos::ssplav::ThreatDetected>();
    std::string result = generateThreatDetectedJson(scan_messages::ThreatDetected(deSerialisedData));

    static const std::string expectedJSON = R"sophos({"details":{"filePath":"","sha256FileHash":"2677b3f1607845d18d5a405a8ef592e79b8a6de355a9b7490b6bb439c2116def"},"detectionName":{"short":"eicar"},"items":{"1":{"path":"","primary":true,"sha256":"2677b3f1607845d18d5a405a8ef592e79b8a6de355a9b7490b6bb439c2116def","type":1}},"threatSource":1,"threatType":1,"time":123})sophos";

    EXPECT_EQ(result, expectedJSON);
}

TEST_F(TestStringUtils, TestEmptyThreatNameJSON)
{

    std::string threatPath = "path/to/threat";
    std::string userID = "User";

    scan_messages::ThreatDetected threatDetected(
        userID,
        DUMMY_TIMESTAMP,
        ThreatType::virus,
        "",
        E_SCAN_TYPE_ON_ACCESS_OPEN,
        E_NOTIFICATION_STATUS_CLEANED_UP,
        threatPath,
        E_SMT_THREAT_ACTION_SHRED,
        "2677b3f1607845d18d5a405a8ef592e79b8a6de355a9b7490b6bb439c2116def",
        m_threatId,
        false,
        ReportSource::ml,
        datatypes::AutoFd());

    std::string dataAsString = threatDetected.serialise();

    const kj::ArrayPtr<const capnp::word> view(
        reinterpret_cast<const capnp::word*>(&(*std::begin(dataAsString))),
        reinterpret_cast<const capnp::word*>(&(*std::end(dataAsString))));

    capnp::FlatArrayMessageReader messageInput(view);
    Sophos::ssplav::ThreatDetected::Reader deSerialisedData =
        messageInput.getRoot<Sophos::ssplav::ThreatDetected>();
    std::string result = generateThreatDetectedJson(scan_messages::ThreatDetected(deSerialisedData));

    static const std::string expectedJSON = R"sophos({"details":{"filePath":"path/to/threat","sha256FileHash":"2677b3f1607845d18d5a405a8ef592e79b8a6de355a9b7490b6bb439c2116def"},"detectionName":{"short":""},"items":{"1":{"path":"path/to/threat","primary":true,"sha256":"2677b3f1607845d18d5a405a8ef592e79b8a6de355a9b7490b6bb439c2116def","type":1}},"threatSource":1,"threatType":1,"time":123})sophos";

    EXPECT_EQ(result, expectedJSON);
}

TEST_F(TestStringUtils, TestGenerateOnAcessConfig)
{
    std::vector<std::string> exclusionList = {"x", "y", "z"};

    std::string expectedResult = R"({"enabled":"true","excludeRemoteFiles":"true","exclusions":["x","y","z"]})";
    EXPECT_EQ(expectedResult, generateOnAccessConfig("true", exclusionList, "true"));

    expectedResult = R"({"enabled":"false","excludeRemoteFiles":"false","exclusions":["x","y","z"]})";
    EXPECT_EQ(expectedResult, generateOnAccessConfig("false", exclusionList, "false"));

    exclusionList = {};
    expectedResult = R"({"enabled":"false","excludeRemoteFiles":"false","exclusions":[]})";
    EXPECT_EQ(expectedResult, generateOnAccessConfig("", exclusionList, ""));

    exclusionList = {};
    expectedResult = R"({"enabled":"false","excludeRemoteFiles":"false","exclusions":[]})";
    EXPECT_EQ(expectedResult, generateOnAccessConfig("this is supposed to be something elese",
                                                     exclusionList,
                                                     "same here"));
}

TEST_F(TestStringUtils, generateCoreCleanEventXmlQuarantineSuccess)
{
    std::string expectedEventXml = R"(<?xml version="1.0" encoding="utf-8"?>
<event type="sophos.core.clean" ts="1970-01-01T00:02:03.000Z">
  <alert id="c1c802c6-a878-ee05-babc-c0378d45d8d4" succeeded="1" origin="0">
    <items totalItems="1">
      <item type="file" result="0">
        <descriptor>/threat/path</descriptor>
      </item>
    </items>
  </alert>
</event>)";

    std::string xml;
    ASSERT_NO_THROW(xml = generateCoreCleanEventXml(m_simpleThreatDetected, common::CentralEnums::QuarantineResult::SUCCESS));
    ASSERT_EQ(xml, expectedEventXml);
}

TEST_F(TestStringUtils, generateCoreCleanEventXmlQuarantineFailedToDeleteFile)
{
    std::string expectedEventXml = R"(<?xml version="1.0" encoding="utf-8"?>
<event type="sophos.core.clean" ts="1970-01-01T00:02:03.000Z">
  <alert id="c1c802c6-a878-ee05-babc-c0378d45d8d4" succeeded="0" origin="0">
    <items totalItems="1">
      <item type="file" result="3">
        <descriptor>/threat/path</descriptor>
      </item>
    </items>
  </alert>
</event>)";

    std::string xml;
    ASSERT_NO_THROW(xml = generateCoreCleanEventXml(m_simpleThreatDetected, common::CentralEnums::QuarantineResult::FAILED_TO_DELETE_FILE));
    ASSERT_EQ(xml, expectedEventXml);
}

TEST_F(TestStringUtils, generateCoreCleanEventXmlFromVdlDetection)
{
    scan_messages::ThreatDetected vdlThreatDetected = scan_messages::ThreatDetected (
        "User ID",
        DUMMY_TIMESTAMP,
        ThreatType::virus,
        "threat name",
        E_SCAN_TYPE_ON_ACCESS_OPEN,
        E_NOTIFICATION_STATUS_CLEANED_UP,
        "/threat/path",
        E_SMT_THREAT_ACTION_SHRED,
        "T2677b3f1607845d",
        m_threatId,
        false,
        ReportSource::vdl,
        datatypes::AutoFd());

    std::string expectedEventXml = R"(<?xml version="1.0" encoding="utf-8"?>
<event type="sophos.core.clean" ts="1970-01-01T00:02:03.000Z">
  <alert id="c1c802c6-a878-ee05-babc-c0378d45d8d4" succeeded="0" origin="1">
    <items totalItems="1">
      <item type="file" result="3">
        <descriptor>/threat/path</descriptor>
      </item>
    </items>
  </alert>
</event>)";

    std::string xml;
    ASSERT_NO_THROW(xml = generateCoreCleanEventXml(vdlThreatDetected, common::CentralEnums::QuarantineResult::FAILED_TO_DELETE_FILE));
    ASSERT_EQ(xml, expectedEventXml);
}