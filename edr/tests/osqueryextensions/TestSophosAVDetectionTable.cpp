/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "MockQueryContext.h"
#include <modules/osqueryextensions/SophosAVDetectionTable.h>

#include <Common/Helpers/TempDir.h>
#include <Common/Helpers/LogInitializedTests.h>
#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>

#include <modules/EventJournalWrapperImpl/EventJournalWrapper.h>

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
    static std::string getPrimaryItemJson()
    {
        return
            R"({"certificates":{"details":{},"isSigned":false,"signerInfo":[]},"cleanUp":true,"isPeFile":true,"path":"/opt/testdir/file.sh","primary":true,"remotePath":false,"sha256":"c88e20178a82af37a51b030cb3797ed144126cad09193a6c8c7e95957cf9c3f9","type":1})";
            ;
    }
};

//TEST_F(TestSophosAVDetectionTable, expectNoThrowWhenGenerateCalled)
//{
//    // add generating data required to generate here
//    MockQueryContext context;
//    EXPECT_NO_THROW(OsquerySDK::SophosAVDetectionTable().Generate(context));
//}

//TEST_F(TestSophosAVDetectionTable, testJsonParsedCorrectly)
//{
//    Tests::TempDir tempDir("/tmp");
//    TableRows results;
//    TableRow r;
//    r["time"] = "123123123";
//    r["rowid"] = "0";
//    r["query_id"] = "query_id_1";
//    r["raw"] = getSampleJson();
//    r["primary_item"] = getPrimaryItemJson();
//    r["primary_item_type"] = "1";
//    r["primary_item_name"] = "/opt/testdir/file.sh";
//    r["detection_thumbprint"] = "c88e20178a82af37a51b030cb3797ed144126cad09193a6c8c7e95957cf9c3f9";
//    r["primary_item_spid"] = "";
//    r["detection_name"] = "ML/PE-A";
//    r["threat_source"] = "ML";
//    r["threat_type"] = "Malware";
//    r["sid"] = "";
//    r["monitor_mode"] = "0";  // optional, ask whether it is a field on its own, or a json field
//    results.push_back(std::move(r));
//
//
//    tempDir.createFile("plugins/edr/etc/testfile.json", getSampleJson());
//    Common::ApplicationConfiguration::applicationConfiguration().setData(
//        Common::ApplicationConfiguration::SOPHOS_INSTALL, tempDir.dirPath());
//
//    MockQueryContext context;
//    std::set<std::string> emptySet = {"query_id_1"};
//    EXPECT_CALL(context, GetConstraints(_ ,_ )).WillOnce(Return(emptySet));
//    EXPECT_EQ(results,OsquerySDK::SophosAVDetectionTable().Generate(context));
//}

//TEST_F(TestSophosAVDetectionTable, test1)
//{
//    Common::EventJournalWrapper::Writer writer("/tmp/event_journal");
//    Common::EventJournalWrapper::Detection detection;
//    detection.data = getSampleJson();
//    detection.subType = "Detections";
//    std::vector<uint8_t> binaryData = writer.encode(detection);
//    writer.insert(Common::EventJournalWrapper::Subject::Detections, binaryData);
//
//
//
//}