/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include <gtest/gtest.h>
#include "ThreatDetected.capnp.h"
#include <scan_messages/ThreatDetected.h>
#include <capnp/serialize.h>
#include "unixsocket/StringUtils.h"


using namespace unixsocket;
using namespace scan_messages;

TEST(TestStringUtils, TestXMLEscapeEscapesControlCharacters) // NOLINT
{
    std::string threatPath = "abc \1 \2 \3 \4 \5 \6 \\ abc \a \b \t \n \v \f \r abc";
    escapeControlCharacters(threatPath);
    EXPECT_EQ(threatPath, "abc \\1 \\2 \\3 \\4 \\5 \\6 \\ abc \\a \\b \\t \\n \\v \\f \\r abc");
}

TEST(TestStringUtils, TestXMLEscapeNotEscapingWeirdCharacters) // NOLINT
{
    std::string threatPath = "ありったけの夢をかき集め \1 \2 \3 \4 \5 \6 \\ Ἄνδρα μοι ἔννεπε \a \b \t \n \v \f \r Ä Ö Ü ß";
    escapeControlCharacters(threatPath);
    EXPECT_EQ(threatPath, "ありったけの夢をかき集め \\1 \\2 \\3 \\4 \\5 \\6 \\ Ἄνδρα μοι ἔννεπε \\a \\b \\t \\n \\v \\f \\r Ä Ö Ü ß");
}

namespace
{
    class TestStringUtilsXML : public ::testing::Test
    {
    public:
        std::string m_englishsXML = R"sophos(<?xml version="1.0" encoding="utf-8"?>
<notification description="Found 'eicar' in 'path/to/threat'" timestamp="19700101 000203" type="sophos.mgt.msg.event.threat" xmlns="http://www.sophos.com/EE/Event">
  <user domain="local" userId="User"/>
  <threat id="1" idSource="1" name="eicar" scanType="201" status="50" type="1">
    <item file="threat" path="path/to/"/>
    <action action="104"/>
  </threat>
</notification>)sophos";

        std::string m_umlatsXML = R"sophos(<?xml version="1.0" encoding="utf-8"?>
<notification description="Found 'Ἄνδρα μοι ἔννεπε, Μοῦσα, πολύτροπον, ὃς μάλα πολλὰ' in '/πλάγχθη, ἐπεὶ Τροίης ἱερὸν πτολίεθρον ἔπερσε·'" timestamp="19700101 000203" type="sophos.mgt.msg.event.threat" xmlns="http://www.sophos.com/EE/Event">
  <user domain="local" userId="πολλῶν δ’ ἀνθρώπων ἴδεν ἄστεα καὶ νόον ἔγνω,, German umlats: Ä Ö Ü ß"/>
  <threat id="1" idSource="1" name="Ἄνδρα μοι ἔννεπε, Μοῦσα, πολύτροπον, ὃς μάλα πολλὰ" scanType="201" status="50" type="1">
    <item file="πλάγχθη, ἐπεὶ Τροίης ἱερὸν πτολίεθρον ἔπερσε·" path="/"/>
    <action action="104"/>
  </threat>
</notification>)sophos";

        std::string m_japaneseXML = R"sophos(<?xml version="1.0" encoding="utf-8"?>
<notification description="Found 'ありったけの夢をかき集め' in '/捜し物を探しに行くのさ ONE PIECE'" timestamp="19700101 000203" type="sophos.mgt.msg.event.threat" xmlns="http://www.sophos.com/EE/Event">
  <user domain="local" userId="羅針盤なんて 渋滞のもと"/>
  <threat id="1" idSource="1" name="ありったけの夢をかき集め" scanType="201" status="50" type="1">
    <item file="捜し物を探しに行くのさ ONE PIECE" path="/"/>
    <action action="104"/>
  </threat>
</notification>)sophos";
    };
    std::time_t m_detectionTimeStamp = 123;
}

TEST_F(TestStringUtilsXML, TestgenerateThreatDetectedXml) // NOLINT
{
    std::string threatName = "eicar";
    std::string threatPath = "path/to/threat";
    std::string userID = "User";

    scan_messages::ThreatDetected threatDetected;
    threatDetected.setUserID(userID);
    threatDetected.setDetectionTime(m_detectionTimeStamp);
    threatDetected.setScanType(E_SCAN_TYPE_ON_ACCESS);
    threatDetected.setThreatType(E_VIRUS_THREAT_TYPE);
    threatDetected.setThreatName(threatName);
    threatDetected.setNotificationStatus(E_NOTIFICATION_STATUS_CLEANED_UP);
    threatDetected.setFilePath(threatPath);
    threatDetected.setActionCode(E_SMT_THREAT_ACTION_SHRED);

    std::string dataAsString = threatDetected.serialise();

    const kj::ArrayPtr<const capnp::word> view(
            reinterpret_cast<const capnp::word*>(&(*std::begin(dataAsString))),
            reinterpret_cast<const capnp::word*>(&(*std::end(dataAsString))));

    capnp::FlatArrayMessageReader messageInput(view);
    Sophos::ssplav::ThreatDetected::Reader deSerialisedData =
            messageInput.getRoot<Sophos::ssplav::ThreatDetected>();

    std::string result = generateThreatDetectedXml(deSerialisedData);

    EXPECT_EQ(result, m_englishsXML);
}

TEST_F(TestStringUtilsXML, TestgenerateThreatDetectedXmlUmlats) // NOLINT
{
    std::string threatName = "Ἄνδρα μοι ἔννεπε, Μοῦσα, πολύτροπον, ὃς μάλα πολλὰ";
    std::string threatPath = "/πλάγχθη, ἐπεὶ Τροίης ἱερὸν πτολίεθρον ἔπερσε·";
    std::string userID = "πολλῶν δ’ ἀνθρώπων ἴδεν ἄστεα καὶ νόον ἔγνω,, German umlats: Ä Ö Ü ß";

    scan_messages::ThreatDetected threatDetected;
    threatDetected.setUserID(userID);
    threatDetected.setDetectionTime(m_detectionTimeStamp);
    threatDetected.setScanType(E_SCAN_TYPE_ON_ACCESS);
    threatDetected.setThreatType(E_VIRUS_THREAT_TYPE);
    threatDetected.setThreatName(threatName);
    threatDetected.setNotificationStatus(E_NOTIFICATION_STATUS_CLEANED_UP);
    threatDetected.setFilePath(threatPath);
    threatDetected.setActionCode(E_SMT_THREAT_ACTION_SHRED);

    std::string dataAsString = threatDetected.serialise();

    const kj::ArrayPtr<const capnp::word> view(
            reinterpret_cast<const capnp::word*>(&(*std::begin(dataAsString))),
            reinterpret_cast<const capnp::word*>(&(*std::end(dataAsString))));

    capnp::FlatArrayMessageReader messageInput(view);
    Sophos::ssplav::ThreatDetected::Reader deSerialisedData =
            messageInput.getRoot<Sophos::ssplav::ThreatDetected>();

    std::string result = generateThreatDetectedXml(deSerialisedData);

    EXPECT_EQ(result, m_umlatsXML);
}

TEST_F(TestStringUtilsXML, TestgenerateThreatDetectedXmlJapaneseCharacters) // NOLINT
{
    std::string threatName = "ありったけの夢をかき集め";
    std::string threatPath = "/捜し物を探しに行くのさ ONE PIECE";
    std::string userID = "羅針盤なんて 渋滞のもと";

    scan_messages::ThreatDetected threatDetected;
    threatDetected.setUserID(userID);
    threatDetected.setDetectionTime(m_detectionTimeStamp);
    threatDetected.setScanType(E_SCAN_TYPE_ON_ACCESS);
    threatDetected.setThreatName(threatName);
    threatDetected.setNotificationStatus(E_NOTIFICATION_STATUS_CLEANED_UP);
    threatDetected.setFilePath(threatPath);
    threatDetected.setActionCode(E_SMT_THREAT_ACTION_SHRED);

    std::string dataAsString = threatDetected.serialise();

    const kj::ArrayPtr<const capnp::word> view(
            reinterpret_cast<const capnp::word*>(&(*std::begin(dataAsString))),
            reinterpret_cast<const capnp::word*>(&(*std::end(dataAsString))));

    capnp::FlatArrayMessageReader messageInput(view);
    Sophos::ssplav::ThreatDetected::Reader deSerialisedData =
            messageInput.getRoot<Sophos::ssplav::ThreatDetected>();

    std::string result = generateThreatDetectedXml(deSerialisedData);

    EXPECT_EQ(result, m_japaneseXML);
}