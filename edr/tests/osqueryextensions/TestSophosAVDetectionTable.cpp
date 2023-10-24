// Copyright 2021-2023 Sophos Limited. All rights reserved.
#include "MockJournalReaderWrapper.h"
#include "MockQueryContext.h"

#include "EventJournalWrapperImpl/EventJournalReaderWrapper.h"
#include "osqueryextensions/SophosAVDetectionTable.h"
#include "osqueryextensions/SophosAVDetectionTableGenerator.h"

#ifdef SPL_BAZEL
#include "tests/Common/Helpers/LogInitializedTests.h"
#include "tests/Common/Helpers/TempDir.h"
#else
#include "Common/Helpers/LogInitializedTests.h"
#include "Common/Helpers/TempDir.h"
#endif

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"

#include <gtest/gtest.h>

using namespace ::testing;

namespace
{
    class TestSophosAVDetectionTable : public LogOffInitializedTests
    {
    public:
        void SetUp() override
        {
            unsetenv("SOPHOS_INSTALL");
        }
        
        static std::string getSampleJson()
        {
            // onaccess detection
            return R"({
               "avScanType":201,
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
                "pid": 100,
                "processPath": "parentPath",
                "quarantineSuccess": true,
                "threatSource": 0,
                "threatType": 1
               })";
        }
        static std::string getOnDemandJson()
        {
            // ondemand detection
            return R"({
               "avScanType":203,
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
                "pid": 100,
                "processPath": "parentPath",
                "quarantineSuccess": true,
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
            return R"({"certificates":{"details":{},"isSigned":false,"signerInfo":[]},"cleanUp":true,"isPeFile":true,"path":"/opt/testdir/file.sh","primary":true,"remotePath":false,"sha256":"c88e20178a82af37a51b030cb3797ed144126cad09193a6c8c7e95957cf9c3f9","type":1})";
            ;
        }

        static std::string getRuntimeDetectionsSampleJson()
        {
            return R"({
    "detectionName": {
        "short": "Suspicious Interactive Shell"
    },
    "detectionThumbprint": "InteractiveShell",
    "threatSource": 7,
    "threatType": 4,
    "time": 1638184256,
    "details": {
        "filePath": "/bin/bash",
        "sha256FileHash": ""
    },
    "sid": "0ca01a43-6f53-4b09-bce0-1d9922262eaa-1467-5681",
    "items": [
        {
            "path": "/bin/bash",
            "primary": true,
            "sha256": "",
            "type": 1,
            "spid": "0ca01a43-6f53-4b09-bce0-1d9922262eaa-1467-5681"
        }
    ],
    "raw": {
        "alert_group_id": "23a09cfb-a814-4e41-bcff-8ad82088c1db",
        "description": "Suspicious Interactive Shell Started",
        "uuid": "73bf97d3-c68b-4dc5-a11c-ffefcf33972f",
        "location": {
            "node_name": "ubuntu1804x64s",
            "kubernetes_pod": "N/A",
            "kubernetes_namespace": "N/A",
            "container_id": "N/A",
            "container_name": "N/A",
            "image_id": "N/A",
            "image_name": "N/A",
            "sensor_id": "runtimedetections-a92ae92c-e34c-4ec4-ad92-a1919490b13f"
        },
        "strategy_name": "Suspicious Interactive Shell",
        "metadata": {
            "arch": "x86_64",
            "capsule8_content_version": "4.9.0.153",
            "capsule8_sensor_build": "616",
            "capsule8_sensor_id": "runtimedetections-a92ae92c-e34c-4ec4-ad92-a1919490b13f",
            "capsule8_sensor_version": "4.9.0.182",
            "collector": "perf",
            "container_runtime": "not-found",
            "cpu_count": "2",
            "has_struct_layouts": "true",
            "in_container": "false",
            "in_host_pid_namespace": "true",
            "investigations_tables": "boot_info,sensor_metadata",
            "kernel_release": "4.15.0-162-generic",
            "kernel_version": "#170-Ubuntu SMP Mon Oct 18 11:38:05 UTC 2021",
            "machine_id": "7e2140083ed19ca2c42dbd489bdff2c5",
            "network_interface_ens5_addr_0": "10.237.165.237/20",
            "network_interface_ens5_addr_0_type": "ip+net",
            "network_interface_ens5_addr_1": "fe80::b1:e0ff:fea3:b85b/64",
            "network_interface_ens5_addr_1_type": "ip+net",
            "network_interface_ens5_flags": "up|broadcast|multicast",
            "network_interface_ens5_hardware_addr": "02:b1:e0:a3:b8:5b",
            "network_interface_ens5_index": "2",
            "network_interface_ens5_mtu": "9001",
            "network_interface_lo_addr_0": "127.0.0.1/8",
            "network_interface_lo_addr_0_type": "ip+net",
            "network_interface_lo_addr_1": "::1/128",
            "network_interface_lo_addr_1_type": "ip+net",
            "network_interface_lo_flags": "up|loopback",
            "network_interface_lo_index": "1",
            "network_interface_lo_mtu": "65536",
            "node_boot_time": "2021-11-29T11:09:59Z",
            "node_hostname": "ubuntu1804x64s",
            "policy_hash": "64bbdc99d1fda4e44c79555aab22ebc9f32b11f07722502ec347d47b761ce3c7",
            "starttime": "2021-11-29T11:10:50.478203454Z",
            "uname_hostname": "ubuntu1804x64s",
            "uname_os": "Linux"
        },
        "categories": [
            "MITRE.Execution.Command and Scripting Interpreter.Unix Shell"
        ]
    }
})";
        }

        static std::string getRuntimeDetectionsPrimaryItemJson()
        {
            return R"({"path":"/bin/bash","primary":true,"sha256":"","spid":"0ca01a43-6f53-4b09-bce0-1d9922262eaa-1467-5681","type":1})";
        }

        static constexpr uint32_t EXPECTED_MAX_MEMORY_THRESHOLD = 50000000;
    };
}

TEST_F(TestSophosAVDetectionTable, testTableGenerationCreatesDataCorrectlyWithNoEventJournalFilesWillReturnEmptyTableCorrectly)
{
    std::string queryId("");
    TableRows expectedResults; // Empty resultset
    std::set<std::string> greaterThanSet = {"0"};
    std::set<std::string> emptySet = {};

    auto MockReaderWrapper = std::make_shared<MockJournalReaderWrapper>();

    std::vector<Common::EventJournalWrapper::Entry> entries = {};
    Common::EventJournalWrapper::Detection detection;
    detection.data = getSampleJson();
    std::pair<bool, Common::EventJournalWrapper::Detection> detectionResult =
        std::make_pair(true, detection);
    NiceMock<MockQueryContext> context;
    EXPECT_CALL(context, GetConstraints("time",_)).WillRepeatedly(Return(emptySet));
    EXPECT_CALL(context, GetConstraints("time",OsquerySDK::GREATER_THAN)).WillOnce(Return(greaterThanSet));
    EXPECT_CALL(context, GetConstraints("query_id",_)).WillOnce(Return(emptySet));
    EXPECT_CALL(*MockReaderWrapper, getCurrentJRLForId(_)).Times(0);
    EXPECT_CALL(*MockReaderWrapper, getEntries(_,0,0,_,_)).WillOnce(Return(entries));
    EXPECT_CALL(*MockReaderWrapper, decode(_)).Times(0);
    EXPECT_CALL(*MockReaderWrapper, updateJrl(_, "jrl")).Times(0);

    OsquerySDK::SophosAVDetectionTableGenerator generator;
    auto actualResults = generator.GenerateData(context, MockReaderWrapper);

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
    r["av_scan_type"] = "on_access";
    r["pid"] = "100";
    r["process_path"] = "parentPath";
    r["quarantine_success"] = "1";
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
    std::set<std::string> greaterThanSet = {"0"};
    std::set<std::string> emptySet = {};
    NiceMock<MockQueryContext> context;
    EXPECT_CALL(context, GetConstraints("time",_)).WillRepeatedly(Return(emptySet));
    EXPECT_CALL(context, GetConstraints("time",OsquerySDK::GREATER_THAN)).WillOnce(Return(greaterThanSet));
    EXPECT_CALL(context, GetConstraints("query_id",_)).WillOnce(Return(emptySet));
    EXPECT_CALL(*MockReaderWrapper, getEntries(_,0,0,_,_)).WillOnce(Return(entries));
    EXPECT_CALL(*MockReaderWrapper, decode(_)).WillRepeatedly(Return(detectionResult));
    OsquerySDK::SophosAVDetectionTableGenerator generator;
    auto actualResults = generator.GenerateData( context, MockReaderWrapper);

    EXPECT_EQ(expectedResults,actualResults);
}

TEST_F(TestSophosAVDetectionTable, testTableGenerationCreatesDataCorrectlyWithOnDemandQuery)
{
    std::string queryId("");
    TableRows expectedResults;
    TableRow r;
    r["time"] = "123123123";
    r["rowid"] = "0";
    r["query_id"] = queryId;
    r["raw"] = getOnDemandJson();
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
    r["av_scan_type"] = "on_demand";
    r["quarantine_success"] = "1";
    expectedResults.push_back(std::move(r));

    auto MockReaderWrapper = std::make_shared<MockJournalReaderWrapper>();

    Common::EventJournalWrapper::Entry entry;
    entry.timestamp = 123123123;
    entry.producerUniqueID = 0;
    entry.data = std::vector<uint8_t> {};
    entry.jrl = "jrl";
    std::vector<Common::EventJournalWrapper::Entry> entries = {entry};
    Common::EventJournalWrapper::Detection detection;
    detection.data = getOnDemandJson();
    std::pair<bool, Common::EventJournalWrapper::Detection> detectionResult =
        std::make_pair(true, detection);
    std::set<std::string> greaterThanSet = {"0"};
    std::set<std::string> emptySet = {};
    NiceMock<MockQueryContext> context;
    EXPECT_CALL(context, GetConstraints("time",_)).WillRepeatedly(Return(emptySet));
    EXPECT_CALL(context, GetConstraints("time",OsquerySDK::GREATER_THAN)).WillOnce(Return(greaterThanSet));
    EXPECT_CALL(context, GetConstraints("query_id",_)).WillOnce(Return(emptySet));
    EXPECT_CALL(*MockReaderWrapper, getEntries(_,0,0,_,_)).WillOnce(Return(entries));
    EXPECT_CALL(*MockReaderWrapper, decode(_)).WillRepeatedly(Return(detectionResult));
    OsquerySDK::SophosAVDetectionTableGenerator generator;
    auto actualResults = generator.GenerateData( context, MockReaderWrapper);

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
        if(i==0)
        {
            r["raw"] = getSampleJson();
            r["av_scan_type"] = "on_access";
            r["pid"] = "100";
            r["process_path"] = "parentPath";
            r["quarantine_success"] = "1";
        }
        else
        {
            r["raw"] = getSampleJson2();
        }
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
    std::set<std::string> greaterThanSet = {"0"};
    std::set<std::string> emptySet = {};
    NiceMock<MockQueryContext> context;
    EXPECT_CALL(context, GetConstraints("time",_)).WillRepeatedly(Return(emptySet));
    EXPECT_CALL(context, GetConstraints("time",OsquerySDK::GREATER_THAN)).WillOnce(Return(greaterThanSet));
    EXPECT_CALL(context, GetConstraints("query_id",_)).WillOnce(Return(emptySet));
    EXPECT_CALL(*MockReaderWrapper, getEntries(_,0,0,_,_)).WillOnce(Return(entries));
    EXPECT_CALL(*MockReaderWrapper, decode(_)).WillOnce(Return(detectionResult2));
    EXPECT_CALL(*MockReaderWrapper, decode(_)).WillOnce(Return(detectionResult1)).RetiresOnSaturation();

    OsquerySDK::SophosAVDetectionTableGenerator generator;
    auto actualResults = generator.GenerateData(context, MockReaderWrapper);

    EXPECT_EQ(expectedResults,actualResults);
}

TEST_F(TestSophosAVDetectionTable, testTableGenerationWithTooManyEntriesThrows)
{
    auto MockReaderWrapper = std::make_shared<MockJournalReaderWrapper>();

    std::vector<Common::EventJournalWrapper::Entry> entries;
    NiceMock<MockQueryContext> context;
    std::set<std::string> greaterThanSet = {"0"};
    std::set<std::string> emptySet = {};
    bool more = true;
    EXPECT_CALL(context, GetConstraints("time",_)).WillRepeatedly(Return(emptySet));
    EXPECT_CALL(context, GetConstraints("time",OsquerySDK::GREATER_THAN)).WillOnce(Return(greaterThanSet));
    EXPECT_CALL(context, GetConstraints("query_id",_)).WillOnce(Return(emptySet));
    EXPECT_CALL(*MockReaderWrapper, getEntries(_,0,0,EXPECTED_MAX_MEMORY_THRESHOLD,_)).WillOnce(DoAll(SetArgReferee<4>(more),Return(entries)));

    OsquerySDK::SophosAVDetectionTableGenerator generator;
    EXPECT_THROW(generator.GenerateData(context, MockReaderWrapper), std::runtime_error);
}

TEST_F(TestSophosAVDetectionTable, testTableGenerationCreatesDataCorrectlyWithQueryIdAndTooManyEntriesReturnsDataCorrectly)
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
    r["av_scan_type"] = "on_access";
    r["pid"] = "100";
    r["process_path"] = "parentPath";
    r["quarantine_success"] = "1";
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
    NiceMock<MockQueryContext> context;
    std::set<std::string> contextSet = {"query_id_1"};
    std::set<std::string> greaterThanSet = {"0"};
    std::set<std::string> emptySet = {};
    bool more = true;
    EXPECT_CALL(context, GetConstraints("time",_)).WillRepeatedly(Return(emptySet));
    EXPECT_CALL(context, GetConstraints("time",OsquerySDK::GREATER_THAN)).WillOnce(Return(greaterThanSet));
    EXPECT_CALL(context, GetConstraints("query_id",_)).WillOnce(Return(contextSet));
    EXPECT_CALL(*MockReaderWrapper, getCurrentJRLForId(_)).WillOnce(Return(""));
    EXPECT_CALL(*MockReaderWrapper, getEntries(_,0,0,EXPECTED_MAX_MEMORY_THRESHOLD,_)).WillOnce(DoAll(SetArgReferee<4>(more),Return(entries)));
    EXPECT_CALL(*MockReaderWrapper, decode(_)).WillRepeatedly(Return(detectionResult));
    EXPECT_CALL(*MockReaderWrapper, updateJrl("/opt/sophos-spl/plugins/edr/var/jrl/query_id_1", "jrl")).Times(1);
    EXPECT_CALL(*MockReaderWrapper, updateJRLAttempts("/opt/sophos-spl/plugins/edr/var/jrl_tracker/query_id_1", 1)).Times(1);

    OsquerySDK::SophosAVDetectionTableGenerator generator;
    auto actualResults = generator.GenerateData(context, MockReaderWrapper);

    EXPECT_EQ(expectedResults,actualResults);
}

TEST_F(TestSophosAVDetectionTable, testTableGenerationClearsJRLWhenTenEventMaxsAreHitForQuery)
{
    std::string queryId("query_id_1");
    std::set<std::string> greaterThanSet = {"0"};
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
    r["monitor_mode"] = "0"; // optional, ask whether it is a field on its own, or a json field
    r["av_scan_type"] = "on_access";
    r["pid"] = "100";
    r["process_path"] = "parentPath";
    r["quarantine_success"] = "1";
    expectedResults.push_back(std::move(r));

    auto MockReaderWrapper = std::make_shared<MockJournalReaderWrapper>();

    Common::EventJournalWrapper::Entry entry;
    entry.timestamp = 123123123;
    entry.producerUniqueID = 0;
    entry.data = std::vector<uint8_t> {};
    entry.jrl = "jrl";
    std::vector<Common::EventJournalWrapper::Entry> entries = { entry };
    Common::EventJournalWrapper::Detection detection;
    detection.data = getSampleJson();
    std::pair<bool, Common::EventJournalWrapper::Detection> detectionResult = std::make_pair(true, detection);
    NiceMock<MockQueryContext> context;
    std::set<std::string> contextSet = { "query_id_1" };
    std::set<std::string> emptySet = {};
    bool more = true;
    EXPECT_CALL(context, GetConstraints("time", _)).WillRepeatedly(Return(emptySet));
    EXPECT_CALL(context, GetConstraints("time",OsquerySDK::GREATER_THAN)).WillOnce(Return(greaterThanSet));
    EXPECT_CALL(context, GetConstraints("query_id", _)).WillOnce(Return(contextSet));
    EXPECT_CALL(*MockReaderWrapper, getCurrentJRLForId(_)).WillOnce(Return(""));
    EXPECT_CALL(*MockReaderWrapper, getCurrentJRLAttemptsForId(_)).WillOnce(Return(10));
    EXPECT_CALL(*MockReaderWrapper, getEntries(_, 0, 0, EXPECTED_MAX_MEMORY_THRESHOLD, _))
        .WillOnce(DoAll(SetArgReferee<4>(more), Return(entries)));
    EXPECT_CALL(*MockReaderWrapper, decode(_)).WillRepeatedly(Return(detectionResult));
    EXPECT_CALL(*MockReaderWrapper, updateJrl("/opt/sophos-spl/plugins/edr/var/jrl/query_id_1", "jrl")).Times(0);
    EXPECT_CALL(*MockReaderWrapper, clearJRLFile("/opt/sophos-spl/plugins/edr/var/jrl/query_id_1")).Times(1);
    EXPECT_CALL(*MockReaderWrapper, clearJRLFile("/opt/sophos-spl/plugins/edr/var/jrl_tracker/query_id_1")).Times(1);

    OsquerySDK::SophosAVDetectionTableGenerator generator;
    auto actualResults = generator.GenerateData(context, MockReaderWrapper);

    EXPECT_EQ(expectedResults, actualResults);
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
    r["av_scan_type"] = "on_access";
    r["pid"] = "100";
    r["process_path"] = "parentPath";
    r["quarantine_success"] = "1";
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
    std::set<std::string> contextSet = {"query_id_1"};
    std::set<std::string> greaterThanSet = {"0"};
    std::set<std::string> emptySet = {};
    NiceMock<MockQueryContext> context;
    EXPECT_CALL(context, GetConstraints("time",_)).WillRepeatedly(Return(emptySet));
    EXPECT_CALL(context, GetConstraints("time",OsquerySDK::GREATER_THAN)).WillOnce(Return(greaterThanSet));
    EXPECT_CALL(context, GetConstraints("query_id",_)).WillOnce(Return(contextSet));
    EXPECT_CALL(*MockReaderWrapper, getCurrentJRLForId(_)).WillOnce(Return(""));
    EXPECT_CALL(*MockReaderWrapper, getEntries(_,0,0,_,_)).WillOnce(Return(entries));
    EXPECT_CALL(*MockReaderWrapper, decode(_)).WillRepeatedly(Return(detectionResult));
    EXPECT_CALL(*MockReaderWrapper, updateJrl("/opt/sophos-spl/plugins/edr/var/jrl/query_id_1", "jrl")).Times(1);
    EXPECT_CALL(*MockReaderWrapper, clearJRLFile("/opt/sophos-spl/plugins/edr/var/jrl_tracker/query_id_1")).Times(1);
    OsquerySDK::SophosAVDetectionTableGenerator generator;
    auto actualResults = generator.GenerateData(context, MockReaderWrapper);

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
    r["av_scan_type"] = "on_access";
    r["pid"] = "100";
    r["process_path"] = "parentPath";
    r["quarantine_success"] = "1";
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
    std::set<std::string> contextSet = {"query_id_1"};
    std::set<std::string> greaterThanSet = {"0"};
    std::set<std::string> emptySet = {};
    NiceMock<MockQueryContext> context;
    EXPECT_CALL(context, GetConstraints("time",_)).WillRepeatedly(Return(emptySet));
    EXPECT_CALL(context, GetConstraints("time",OsquerySDK::GREATER_THAN)).WillOnce(Return(greaterThanSet));
    EXPECT_CALL(context, GetConstraints("query_id",_)).WillOnce(Return(contextSet));
    EXPECT_CALL(*MockReaderWrapper, getCurrentJRLForId(_)).WillOnce(Return("current_jrl"));
    EXPECT_CALL(*MockReaderWrapper, getEntries(_,"current_jrl",_,_)).WillOnce(Return(entries));
    EXPECT_CALL(*MockReaderWrapper, decode(_)).WillRepeatedly(Return(detectionResult));
    EXPECT_CALL(*MockReaderWrapper, updateJrl("/opt/sophos-spl/plugins/edr/var/jrl/query_id_1", "jrl")).Times(1);
    OsquerySDK::SophosAVDetectionTableGenerator generator;
    auto actualResults = generator.GenerateData(context, MockReaderWrapper);

    EXPECT_EQ(expectedResults,actualResults);
}

TEST_F(TestSophosAVDetectionTable, testTableGenerationCreatesDataCorrectlyWithRuntimeDetections)
{
    TableRows expectedResults;
    TableRow r;
    r["time"] = "123123123";
    r["rowid"] = "456";
    r["query_id"] = "";
    r["raw"] = getRuntimeDetectionsSampleJson();
    r["primary_item"] = getRuntimeDetectionsPrimaryItemJson();
    r["primary_item_name"] = "/bin/bash";
    r["primary_item_spid"] = "0ca01a43-6f53-4b09-bce0-1d9922262eaa-1467-5681";
    r["primary_item_type"] = "FILE";
    r["detection_name"] = "Suspicious Interactive Shell";
    r["detection_thumbprint"] = "InteractiveShell";
    r["sid"] = "0ca01a43-6f53-4b09-bce0-1d9922262eaa-1467-5681";
    r["threat_source"] = "Behavioral";
    r["threat_type"] = "Suspicious Behaviour";
    r["monitor_mode"] = "0";
    expectedResults.push_back(std::move(r));

    auto MockReaderWrapper = std::make_shared<MockJournalReaderWrapper>();

    Common::EventJournalWrapper::Entry entry;
    entry.timestamp = 123123123;
    entry.producerUniqueID = 456;
    std::vector<Common::EventJournalWrapper::Entry> entries = {entry};
    Common::EventJournalWrapper::Detection detection;
    detection.data = getRuntimeDetectionsSampleJson();
    std::set<std::string> greaterThanSet = {"0"};
    std::set<std::string> emptySet = {};
    NiceMock<MockQueryContext> context;
    EXPECT_CALL(context, GetConstraints("time",_)).WillRepeatedly(Return(emptySet));
    EXPECT_CALL(context, GetConstraints("time",OsquerySDK::GREATER_THAN)).WillOnce(Return(greaterThanSet));
    EXPECT_CALL(context, GetConstraints("query_id",_)).WillOnce(Return(emptySet));
    EXPECT_CALL(*MockReaderWrapper, getEntries(_,_,_,_,_)).WillOnce(Return(entries));
    EXPECT_CALL(*MockReaderWrapper, decode(_)).WillOnce(Return(std::make_pair(true, detection)));

    OsquerySDK::SophosAVDetectionTableGenerator generator;
    auto actualResults = generator.GenerateData(context, MockReaderWrapper);

    EXPECT_EQ(expectedResults, actualResults);
}
