/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "MockJournalReaderWrapper.h"
#include "MockQueryContext.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/Helpers/LogInitializedTests.h>
#include <Common/Helpers/TempDir.h>
#include <modules/EventJournalWrapperImpl/EventJournalReaderWrapper.h>
#include <osqueryextensions/SophosAVDetectionTableGenerator.h>

#include <gtest/gtest.h>

using namespace ::testing;

class TestSophosAVDetectionTable : public LogOffInitializedTests
{
public:

    static std::string getSampleJson()
    {
        return R"({
               "details": {
               "time": 123123123,
               "certificates": {},
               "fileMetadata": {},
               "filePath": "/opt/testdir/file.sh",
               "fileSize": 660,
               "globalReputation": 62,
               "localReputation": -1,
               "localReputationConfigVersion": "e9f53340ca6598d87ba24a832311b9c61fb486d9bbf9b25c53b6c3d4fdd7578c",
               "localReputationSampleRate": 0,
               "mlScoreMain": 100,
               "mlScorePua": 100,
               "mlSuppressed": false,
               "remote": false,
               "reputationSource": "SXL4_LOOKUP",
               "savScanResult": "",
               "scanEvalReason": "PINNED_CACHE_BLOCK_NOTIFY",
               "sha1FileHash": "46763aa950d6f2a3615cc63226afb5180b5a229b",
               "sha256FileHash": "c88e20178a82af37a51b030cb3797ed144126cad09193a6c8c7e95957cf9c3f9",
               "sting20Enabled": false
                },
                "detectionName": {
                    "short": "ML/PE-A"
                },
                "flags": {
                    "isSuppressible": false,
                    "isUserVisible": true,
                    "sendVdlTelemetry": true,
                    "useClassicTelemetry": true,
                    "useSavScanResults": true
                },
                "items": {
                    "1": {
                        "certificates": {"details": {}, "isSigned": false, "signerInfo": []},
                        "cleanUp": true,
                        "isPeFile": true,
                        "path": "/opt/testdir/file.sh",
                        "primary": true,
                        "remotePath": false,
                        "sha256": "c88e20178a82af37a51b030cb3797ed144126cad09193a6c8c7e95957cf9c3f9",
                        "type": 1
                    }
                },
                "threatSource": 0,
                "threatType": 1
               })";
    }

    static std::string getSampleJson2()
    {
        return R"({
               "details": {
               "time": 123123123,
               "certificates": {},
               "fileMetadata": {},
               "filePath": "/opt/testdir/file.sh",
               "fileSize": 660,
               "globalReputation": 62,
               "localReputation": -1,
               "localReputationConfigVersion": "e9f53340ca6598d87ba24a832311b9c61fb486d9bbf9b25c53b6c3d4fdd7578c",
               "localReputationSampleRate": 0,
               "mlScoreMain": 100,
               "mlScorePua": 100,
               "mlSuppressed": false,
               "remote": false,
               "reputationSource": "SXL4_LOOKUP",
               "savScanResult": "",
               "scanEvalReason": "PINNED_CACHE_BLOCK_NOTIFY",
               "sha1FileHash": "46763aa950d6f2a3615cc63226afb5180b5a229b",
               "sha256FileHash": "c88e20178a82af37a51b030cb3797ed144126cad09193a6c8c7e95957cf9c3f9",
               "sting20Enabled": false
                },
                "detectionName": {
                    "short": "ML/PE-A"
                },
                "flags": {
                    "isSuppressible": false,
                    "isUserVisible": true,
                    "sendVdlTelemetry": true,
                    "useClassicTelemetry": true,
                    "useSavScanResults": true
                },
                "items": {
                    "1": {
                        "certificates": {"details": {}, "isSigned": false, "signerInfo": []},
                        "cleanUp": true,
                        "isPeFile": true,
                        "path": "/opt/testdir/file.sh",
                        "primary": true,
                        "remotePath": false,
                        "sha256": "c88e20178a82af37a51b030cb3797ed144126cad09193a6c8c7e95957cf9c3f9",
                        "type": 1
                    }
                },
                "threatSource": 0,
                "threatType": 1
               })";
    }

    static std::string getPrimaryItemJson()
    {
        return
            R"({"certificates":{"details":{},"isSigned":false,"signerInfo":[]},"cleanUp":true,"isPeFile":true,"path":"/opt/testdir/file.sh","primary":true,"remotePath":false,"sha256":"c88e20178a82af37a51b030cb3797ed144126cad09193a6c8c7e95957cf9c3f9","type":1})";
            ;
    }
};

TEST_F(TestSophosAVDetectionTable, testTableGenerationCreatesDataCorrectlyWithNoEventJournalFilesWillReturnEmptyTableCorrectly)
{
    std::string queryId("");
    TableRows expectedResults; // Empty resultset

    auto MockReaderWrapper = std::make_shared<MockJournalReaderWrapper>();

    std::vector<Common::EventJournalWrapper::Entry> entries = {};
    Common::EventJournalWrapper::Detection detection;
    detection.data = getSampleJson();
    std::pair<bool, Common::EventJournalWrapper::Detection> detectionResult =
        std::make_pair(true, detection);

    EXPECT_CALL(*MockReaderWrapper, getCurrentJRLForId(_)).Times(0);
    EXPECT_CALL(*MockReaderWrapper, getEntries(_,0,0,0)).WillOnce(Return(entries));
    EXPECT_CALL(*MockReaderWrapper, decode(_)).Times(0);
    EXPECT_CALL(*MockReaderWrapper, updateJrl(_, "jrl")).Times(0);
    OsquerySDK::SophosAVDetectionTableGenerator generator;
    auto actualResults = generator.GenerateData(queryId, MockReaderWrapper);

    EXPECT_EQ(expectedResults,actualResults);
}

TEST_F(TestSophosAVDetectionTable, testTableGenerationCreatesDataCorrectlyWithNoQueryIdReturnsDataCorrectly)
{
    std::string queryId("");
    TableRows expectedResults;
    TableRow r;
    r["time"] = "123123123";
    r["rowid"] = "0";
    r["query_id"] = queryId;
    r["raw"] = getSampleJson();
    r["primary_item"] = getPrimaryItemJson();
    r["primary_item_type"] = "FILE";
    r["primary_item_name"] = "/opt/testdir/file.sh";
    r["detection_thumbprint"] = "c88e20178a82af37a51b030cb3797ed144126cad09193a6c8c7e95957cf9c3f9";
    r["primary_item_spid"] = "";
    r["detection_name"] = "ML/PE-A";
    r["threat_source"] = "ML";
    r["threat_type"] = "Malware";
    r["sid"] = "";
    r["monitor_mode"] = "0";  // optional, ask whether it is a field on its own, or a json field
    expectedResults.push_back(std::move(r));

    auto MockReaderWrapper = std::make_shared<MockJournalReaderWrapper>();

    Common::EventJournalWrapper::Entry entry;
    entry.timestamp = 123123123;
    entry.producerUniqueID = 0;
    entry.data = std::vector<uint8_t> {};
    entry.jrl = "jrl";
    std::vector<Common::EventJournalWrapper::Entry> entries = {entry};
    Common::EventJournalWrapper::Detection detection;
    detection.data = getSampleJson();
    std::pair<bool, Common::EventJournalWrapper::Detection> detectionResult =
        std::make_pair(true, detection);

    EXPECT_CALL(*MockReaderWrapper, getEntries(_,0,0,0)).WillOnce(Return(entries));
    EXPECT_CALL(*MockReaderWrapper, decode(_)).WillRepeatedly(Return(detectionResult));
    OsquerySDK::SophosAVDetectionTableGenerator generator;
    auto actualResults = generator.GenerateData(queryId, MockReaderWrapper);

    EXPECT_EQ(expectedResults,actualResults);
}

TEST_F(TestSophosAVDetectionTable, testTableGenerationCreatesDataCorrectlyWithMultipleEntriesReturnsDataCorrectly)
{
    std::string queryId("");
    TableRows expectedResults;
    for(int i=0; i<2; i++)
    {
        TableRow r;
        r["time"] = "123123123";
        r["rowid"] = "0";
        r["query_id"] = queryId;
        r["raw"] = getSampleJson();
        r["primary_item"] = getPrimaryItemJson();
        r["primary_item_type"] = "FILE";
        r["primary_item_name"] = "/opt/testdir/file.sh";
        r["detection_thumbprint"] = "c88e20178a82af37a51b030cb3797ed144126cad09193a6c8c7e95957cf9c3f9";
        r["primary_item_spid"] = "";
        r["detection_name"] = "ML/PE-A";
        r["threat_source"] = "ML";
        r["threat_type"] = "Malware";
        r["sid"] = "";
        r["monitor_mode"] = "0"; // optional, ask whether it is a field on its own, or a json field
        expectedResults.push_back(std::move(r));
    }

    auto MockReaderWrapper = std::make_shared<MockJournalReaderWrapper>();

    Common::EventJournalWrapper::Entry entry1;
    Common::EventJournalWrapper::Entry entry2;
    entry1.timestamp = 123123123;
    entry1.producerUniqueID = 0;
    entry1.data = std::vector<uint8_t> {};
    entry1.jrl = "jrl1";
    entry2.timestamp = 123123345;
    entry2.producerUniqueID = 0;
    entry2.data = std::vector<uint8_t> {};
    entry2.jrl = "jrl2";
    std::vector<Common::EventJournalWrapper::Entry> entries = {entry1, entry2};

    Common::EventJournalWrapper::Detection detection1;
    detection1.data = getSampleJson();
    std::pair<bool, Common::EventJournalWrapper::Detection> detectionResult1 =
        std::make_pair(true, detection1);
    Common::EventJournalWrapper::Detection detection2;
    detection2.data = getSampleJson2();
    std::pair<bool, Common::EventJournalWrapper::Detection> detectionResult2 =
        std::make_pair(true, detection2);

    EXPECT_CALL(*MockReaderWrapper, getEntries(_,0,0,0)).WillOnce(Return(entries));
    EXPECT_CALL(*MockReaderWrapper, decode(_)).WillOnce(Return(detectionResult2));
    EXPECT_CALL(*MockReaderWrapper, decode(_)).WillOnce(Return(detectionResult1)).RetiresOnSaturation();

    OsquerySDK::SophosAVDetectionTableGenerator generator;
    auto actualResults = generator.GenerateData(queryId, MockReaderWrapper);

    EXPECT_EQ(expectedResults,actualResults);
}

TEST_F(TestSophosAVDetectionTable, testTableGenerationCreatesDataCorrectlyWithQueryIdReturnsDataCorrectly)
{
    std::string queryId("query_id_1");
    TableRows expectedResults;
    TableRow r;
    r["time"] = "123123123";
    r["rowid"] = "0";
    r["query_id"] = queryId;
    r["raw"] = getSampleJson();
    r["primary_item"] = getPrimaryItemJson();
    r["primary_item_type"] = "FILE";
    r["primary_item_name"] = "/opt/testdir/file.sh";
    r["detection_thumbprint"] = "c88e20178a82af37a51b030cb3797ed144126cad09193a6c8c7e95957cf9c3f9";
    r["primary_item_spid"] = "";
    r["detection_name"] = "ML/PE-A";
    r["threat_source"] = "ML";
    r["threat_type"] = "Malware";
    r["sid"] = "";
    r["monitor_mode"] = "0";  // optional, ask whether it is a field on its own, or a json field
    expectedResults.push_back(std::move(r));

    auto MockReaderWrapper = std::make_shared<MockJournalReaderWrapper>();

    Common::EventJournalWrapper::Entry entry;
    entry.timestamp = 123123123;
    entry.producerUniqueID = 0;
    entry.data = std::vector<uint8_t> {};
    entry.jrl = "jrl";
    std::vector<Common::EventJournalWrapper::Entry> entries = {entry};
    Common::EventJournalWrapper::Detection detection;
    detection.data = getSampleJson();
    std::pair<bool, Common::EventJournalWrapper::Detection> detectionResult =
        std::make_pair(true, detection);

    EXPECT_CALL(*MockReaderWrapper, getCurrentJRLForId(_)).WillOnce(Return(""));
    EXPECT_CALL(*MockReaderWrapper, getEntries(_,0,0,0)).WillOnce(Return(entries));
    EXPECT_CALL(*MockReaderWrapper, decode(_)).WillRepeatedly(Return(detectionResult));
    EXPECT_CALL(*MockReaderWrapper, updateJrl("/opt/sophos-spl/plugins/edr/var/jrl/query_id_1", "jrl")).Times(1);
    OsquerySDK::SophosAVDetectionTableGenerator generator;
    auto actualResults = generator.GenerateData(queryId, MockReaderWrapper);

    EXPECT_EQ(expectedResults,actualResults);
}

TEST_F(TestSophosAVDetectionTable, testTableGenerationCreatesDataCorrectlyWithQueryIdAndJrlReturnsDataCorrectly)
{
    std::string queryId("query_id_1");
    TableRows expectedResults;
    TableRow r;
    r["time"] = "123123123";
    r["rowid"] = "0";
    r["query_id"] = queryId;
    r["raw"] = getSampleJson();
    r["primary_item"] = getPrimaryItemJson();
    r["primary_item_type"] = "FILE";
    r["primary_item_name"] = "/opt/testdir/file.sh";
    r["detection_thumbprint"] = "c88e20178a82af37a51b030cb3797ed144126cad09193a6c8c7e95957cf9c3f9";
    r["primary_item_spid"] = "";
    r["detection_name"] = "ML/PE-A";
    r["threat_source"] = "ML";
    r["threat_type"] = "Malware";
    r["sid"] = "";
    r["monitor_mode"] = "0";  // optional, ask whether it is a field on its own, or a json field
    expectedResults.push_back(std::move(r));

    auto MockReaderWrapper = std::make_shared<MockJournalReaderWrapper>();

    Common::EventJournalWrapper::Entry entry;
    entry.timestamp = 123123123;
    entry.producerUniqueID = 0;
    entry.data = std::vector<uint8_t> {};
    entry.jrl = "jrl";
    std::vector<Common::EventJournalWrapper::Entry> entries = {entry};
    Common::EventJournalWrapper::Detection detection;
    detection.data = getSampleJson();
    std::pair<bool, Common::EventJournalWrapper::Detection> detectionResult =
        std::make_pair(true, detection);

    EXPECT_CALL(*MockReaderWrapper, getCurrentJRLForId(_)).WillOnce(Return("current_jrl"));
    EXPECT_CALL(*MockReaderWrapper, getEntries(_,"current_jrl")).WillOnce(Return(entries));
    EXPECT_CALL(*MockReaderWrapper, decode(_)).WillRepeatedly(Return(detectionResult));
    EXPECT_CALL(*MockReaderWrapper, updateJrl("/opt/sophos-spl/plugins/edr/var/jrl/query_id_1", "jrl")).Times(1);
    OsquerySDK::SophosAVDetectionTableGenerator generator;
    auto actualResults = generator.GenerateData(queryId, MockReaderWrapper);

    EXPECT_EQ(expectedResults,actualResults);
}
