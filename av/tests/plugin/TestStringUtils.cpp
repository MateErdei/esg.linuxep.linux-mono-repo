// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include "ThreatDetected.capnp.h"

#include "pluginimpl/StringUtils.h"
#include "tests/common/LogInitializedTests.h"

#include <capnp/serialize.h>
#include <gtest/gtest.h>
#include <scan_messages/ThreatDetected.h>

using namespace pluginimpl;
using namespace scan_messages;

namespace
{
    class TestStringUtils : public LogInitializedTests
    {
    public:
        std::string sha256 = "590165d5fd84b3302e239c260867a2310af2bc3b519b5c9c68ab2515c9bad15b";
        std::string threatId = "Tc1c802c6a878ee05babcc0378d45d8d449a06784c14508f7200a63323ca0a350";

        std::string m_englishsXML = R"sophos(<?xml version="1.0" encoding="utf-8"?>
<notification description="Found 'eicar' in 'path/to/threat'" timestamp="19700101 000203" type="sophos.mgt.msg.event.threat" xmlns="http://www.sophos.com/EE/Event">
  <user domain="local" userId="User"/>
  <threat id="Tc1c802c6a878ee05babcc0378d45d8d449a06784c14508f7200a63323ca0a350" idSource="Tsha256(path,name)" name="eicar" scanType="201" status="50" type="1">
    <item file="threat" path="path/to/"/>
    <action action="104"/>
  </threat>
</notification>)sophos";

        std::string m_umlatsXML = R"sophos(<?xml version="1.0" encoding="utf-8"?>
<notification description="Found 'Ἄνδρα μοι ἔννεπε, Μοῦσα, πολύτροπον, ὃς μάλα πολλὰ' in '/πλάγχθη, ἐπεὶ Τροίης ἱερὸν πτολίεθρον ἔπερσε·'" timestamp="19700101 000203" type="sophos.mgt.msg.event.threat" xmlns="http://www.sophos.com/EE/Event">
  <user domain="local" userId="πολλῶν δ’ ἀνθρώπων ἴδεν ἄστεα καὶ νόον ἔγνω,, German umlats: Ä Ö Ü ß"/>
  <threat id="Tc1c802c6a878ee05babcc0378d45d8d449a06784c14508f7200a63323ca0a350" idSource="Tsha256(path,name)" name="Ἄνδρα μοι ἔννεπε, Μοῦσα, πολύτροπον, ὃς μάλα πολλὰ" scanType="201" status="50" type="1">
    <item file="πλάγχθη, ἐπεὶ Τροίης ἱερὸν πτολίεθρον ἔπερσε·" path="/"/>
    <action action="104"/>
  </threat>
</notification>)sophos";

        std::string m_japaneseXML = R"sophos(<?xml version="1.0" encoding="utf-8"?>
<notification description="Found 'ありったけの夢をかき集め' in '/捜し物を探しに行くのさ ONE PIECE'" timestamp="19700101 000203" type="sophos.mgt.msg.event.threat" xmlns="http://www.sophos.com/EE/Event">
  <user domain="local" userId="羅針盤なんて 渋滞のもと"/>
  <threat id="Tc1c802c6a878ee05babcc0378d45d8d449a06784c14508f7200a63323ca0a350" idSource="Tsha256(path,name)" name="ありったけの夢をかき集め" scanType="201" status="50" type="1">
    <item file="捜し物を探しに行くのさ ONE PIECE" path="/"/>
    <action action="104"/>
  </threat>
</notification>)sophos";
    };
    std::time_t m_detectionTimeStamp = 123;
}

TEST_F(TestStringUtils, TestgenerateThreatDetectedXml)
{
    std::string threatName = "eicar";
    std::string threatPath = "path/to/threat";
    std::string userID = "User";

    scan_messages::ThreatDetected threatDetected(
        userID,
        m_detectionTimeStamp,
        E_VIRUS_THREAT_TYPE,
        threatName,
        E_SCAN_TYPE_ON_ACCESS,
        E_NOTIFICATION_STATUS_CLEANED_UP,
        threatPath,
        E_SMT_THREAT_ACTION_SHRED,
        sha256,
        threatId,
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
        m_detectionTimeStamp,
        E_VIRUS_THREAT_TYPE,
        threatName,
        E_SCAN_TYPE_ON_ACCESS,
        E_NOTIFICATION_STATUS_CLEANED_UP,
        threatPath,
        E_SMT_THREAT_ACTION_SHRED,
        sha256,
        threatId,
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
        m_detectionTimeStamp,
        E_VIRUS_THREAT_TYPE,
        threatName,
        E_SCAN_TYPE_ON_ACCESS,
        E_NOTIFICATION_STATUS_CLEANED_UP,
        threatPath,
        E_SMT_THREAT_ACTION_SHRED,
        sha256,
        threatId,
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
    const std::string& sha256 = "",
    const std::string& threatId = ""
    )
{
    scan_messages::ThreatDetected threatDetected(
        userID,
        m_detectionTimeStamp,
        E_VIRUS_THREAT_TYPE,
        threatName,
        E_SCAN_TYPE_ON_ACCESS,
        E_NOTIFICATION_STATUS_CLEANED_UP,
        threatPath,
        E_SMT_THREAT_ACTION_SHRED,
        sha256,
        threatId,
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
<notification description="Found '' in ''" timestamp="19700101 000203" type="sophos.mgt.msg.event.threat" xmlns="http://www.sophos.com/EE/Event">
  <user domain="local" userId=""/>
  <threat id="" idSource="Tsha256(path,name)" name="" scanType="201" status="50" type="1">
    <item file="" path=""/>
    <action action="104"/>
  </threat>
</notification>)sophos";

    EXPECT_EQ(result, expectedXML);
}

TEST_F(TestStringUtils, TestEmptyThreatPathJSON)
{
    std::string threatName = "eicar";
    std::string userID = "User";

    scan_messages::ThreatDetected threatDetected(
        userID,
        m_detectionTimeStamp,
        E_VIRUS_THREAT_TYPE,
        threatName,
        E_SCAN_TYPE_ON_ACCESS_OPEN,
        E_NOTIFICATION_STATUS_CLEANED_UP,
        "",
        E_SMT_THREAT_ACTION_SHRED,
        "2677b3f1607845d18d5a405a8ef592e79b8a6de355a9b7490b6bb439c2116def",
        threatId,
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
        m_detectionTimeStamp,
        E_VIRUS_THREAT_TYPE,
        "",
        E_SCAN_TYPE_ON_ACCESS_OPEN,
        E_NOTIFICATION_STATUS_CLEANED_UP,
        threatPath,
        E_SMT_THREAT_ACTION_SHRED,
        "2677b3f1607845d18d5a405a8ef592e79b8a6de355a9b7490b6bb439c2116def",
        threatId,
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