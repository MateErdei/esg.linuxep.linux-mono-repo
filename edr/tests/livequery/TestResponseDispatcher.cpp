// Copyright 2019-2023 Sophos Limited. All rights reserved.

// File under test
#include "livequery/ResponseDispatcher.h"

// Product
#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFileSystem.h"

// 3rd party production
#include <nlohmann/json.hpp>

// Sophos test

#ifdef SPL_BAZEL
#include "tests/Common/Helpers/LogInitializedTests.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/MockFilePermissions.h"
#include "tests/Common/Helpers/MockFileSystem.h"
#else
#include "Common/Helpers/LogInitializedTests.h"
#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/MockFilePermissions.h"
#include "Common/Helpers/MockFileSystem.h"
#endif


// Third party test
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

using namespace ::testing;
using namespace livequery;

ResponseData::ColumnHeaders headerExample()
{
    ResponseData::ColumnHeaders  headers;
    headers.emplace_back("pathname", OsquerySDK::ColumnType::TEXT_TYPE);
    headers.emplace_back("sophosPID",OsquerySDK::ColumnType::TEXT_TYPE);
    headers.emplace_back("start_time", OsquerySDK::ColumnType::BIGINT_TYPE);
    return headers;
}


/** Json strings can have any order by the standard. Besides, indentation and spaces may change,
 * which makes string comparison to validade equivalence not valid.
 * This method relies on the equality of nlohmann::json to verify that two strings produces equivalent
 * json
 * @param v1: serialized json (must be valid)
 * @param v2: serialized json (must be valid)
 * @return  true if v1 and v2 describe the same values and keys.
 */
bool serializedJsonContentAreEquivalent(const std::string & v1, const std::string & v2)
{
    nlohmann::json jv1 = nlohmann::json::parse(v1);
    nlohmann::json jv2 = nlohmann::json::parse(v2);
    std::string str1 = to_string(jv1);
    std::string str2 = to_string(jv2);

    if (str1 == str2)
    {
        return true;
    }

    return jv1 == jv2;
}

namespace
{
    class TestResponseDispatcher : public LogOffInitializedTests
    {
    };
}

TEST_F(TestResponseDispatcher, verifySerializedJsonAreEquivalent)
{
    std::string v1 = R"({"data":{"a":"a","b":1}})";
    std::string v2 = R"({"data":{"b":1,"a":"a"}})";
    std::string v3 = R"({"data":{"b":"1","a":"a"}})";
    EXPECT_TRUE (serializedJsonContentAreEquivalent(v1,v2));
    EXPECT_FALSE (serializedJsonContentAreEquivalent(v1,v3));
    EXPECT_FALSE (serializedJsonContentAreEquivalent(v2,v3));
}

TEST_F(TestResponseDispatcher, JsonForExceededEntriesShouldNotIncludeDataColumns)
{
    QueryResponse response{
        ResponseStatus{ErrorCode::RESPONSEEXCEEDLIMIT},
        ResponseData{headerExample(), ResponseData::MarkDataExceeded::DataExceedLimit},
        ResponseMetaData()};

    std::string expected = R"({
    "type": "sophos.mgt.response.RunLiveQuery",
    "queryMetaData": {
        "errorCode": 100,
        "errorMessage": "Response data exceeded 10MB",
        "sizeBytes" : 0
    },
    "columnMetaData": [
      {"name": "pathname", "type": "TEXT"},
      {"name": "sophosPID", "type": "TEXT"},
      {"name": "start_time", "type": "BIGINT"}
    ]
})";
    ResponseDispatcher dispatcher;
    std::string calculated = dispatcher.serializeToJson(response);
    EXPECT_TRUE(serializedJsonContentAreEquivalent(expected, calculated)) << calculated;
}

TEST_F(TestResponseDispatcher, SpecificErrorConditionShouldBeIncludedInTheJsonFile)
{
    ResponseStatus status{ErrorCode::OSQUERYERROR};
    status.overrideErrorDescription("SpecialError condition reported by osquery");
    QueryResponse response{status,
                           ResponseData{headerExample(), ResponseData::ColumnData{}},
                           ResponseMetaData()};
    std::string expected = R"({
    "type": "sophos.mgt.response.RunLiveQuery",
    "queryMetaData": {
        "errorCode": 1,
        "errorMessage": "SpecialError condition reported by osquery",
        "sizeBytes": 0
    },
    "columnMetaData": [
      {"name": "pathname", "type": "TEXT"},
      {"name": "sophosPID", "type": "TEXT"},
      {"name": "start_time", "type": "BIGINT"}
    ]
})";
    ResponseDispatcher dispatcher;
    std::string calculated = dispatcher.serializeToJson(response);
    EXPECT_TRUE(serializedJsonContentAreEquivalent(expected, calculated)) << "\nCalculated: "<< calculated;
}


TEST_F(TestResponseDispatcher, emptyResponseWhereNotRowWasSelectedShouldReturnExpectedJson)
{
    QueryResponse response{ResponseStatus{ErrorCode::SUCCESS},
                           ResponseData{headerExample(), ResponseData::ColumnData{}},
                           ResponseMetaData()};
    std::string expected = R"({
    "type": "sophos.mgt.response.RunLiveQuery",
    "queryMetaData": {
        "rows": 0,
        "errorCode": 0,
        "errorMessage": "OK",
        "sizeBytes" : 0
    },
    "columnMetaData": [
      {"name": "pathname", "type": "TEXT"},
      {"name": "sophosPID", "type": "TEXT"},
      {"name": "start_time", "type": "BIGINT"}
    ],
    "columnData":[]
})";
    ResponseDispatcher dispatcher;
    std::string calculated = dispatcher.serializeToJson(response);
    EXPECT_TRUE(serializedJsonContentAreEquivalent(expected, calculated))<< "\nCalculated: "<< calculated;
}

TEST_F(TestResponseDispatcher, testColumnMetaDataIsPresentWhenNoHeadersAndColumnDataFieldIsPresent)
{
    ResponseData::ColumnHeaders  empty_headers {};


    QueryResponse response{ResponseStatus{ErrorCode::SUCCESS},
                           ResponseData{empty_headers, ResponseData::ColumnData{}},
                           ResponseMetaData()};

    std::string expected = R"({
    "type": "sophos.mgt.response.RunLiveQuery",
    "queryMetaData": {
        "errorCode": 0,
        "errorMessage": "OK",
        "sizeBytes" : 0
    },
    "columnMetaData":[],
    "columnData":[]
})";
    ResponseDispatcher dispatcher;
    std::string calculated = dispatcher.serializeToJson(response);
    EXPECT_TRUE(serializedJsonContentAreEquivalent(expected, calculated))<< "\nCalculated: "<< calculated << "\nExpected: "<< expected;
}

TEST_F(TestResponseDispatcher, extendedValidQueryResponseShouldReturnExpectedJson)
{
    //create response data given a real osquery response as below
    /*
    std::string realOSqueryResonse = R"(
    {
        "action": "added",
        "columns": {
        "time": "1518221620",
        "pid": "21306",
        "path": "/usr/bin/someexe",
        "mode": "1000755"
        "lastAccess": "Tue Jan 14 15:53:13 2020 UTC",
        },
        "UnixTime": "1518223620",
        "calendarTime": "Tue Jan 14 17:53:13 2020 UTC",
        "name": "process_events"
    })";
     */

    ResponseData::ColumnHeaders  headers;
    headers.emplace_back("time", OsquerySDK::ColumnType::BIGINT_TYPE);
    headers.emplace_back("pid", OsquerySDK::ColumnType::INTEGER_TYPE);
    headers.emplace_back("mode", OsquerySDK::ColumnType::UNSIGNED_BIGINT_TYPE);
    headers.emplace_back("path", OsquerySDK::ColumnType::TEXT_TYPE);
    headers.emplace_back("uid", OsquerySDK::ColumnType::INTEGER_TYPE);
    headers.emplace_back("lastAccess", OsquerySDK::ColumnType::TEXT_TYPE);


    ResponseData::ColumnData columnData;
    ResponseData::RowData  rowData;
    rowData["time"] = "1518221620";
    rowData["pid"] = "21306";
    rowData["mode"] = "1000755";
    rowData["path"] = "/usr/bin/someexe";
    rowData["uid"] = "500";
    rowData["lastAccess"] = "Tue Jan 14 15:53:13 2020 UTC";
    columnData.push_back(rowData);
    // difference between first and second row is the pid,path, and empty mode empty time
    rowData["pid"] = "50331";
    rowData["mode"] = "0";
    rowData["path"] = "/usr/bin/someOther.exe";
    rowData["uid"] = "";
    columnData.push_back(rowData);


    QueryResponse response{ResponseStatus{ErrorCode::SUCCESS},
                           ResponseData{headers, columnData},
                           ResponseMetaData()};
    std::string expected = R"({
    "type": "sophos.mgt.response.RunLiveQuery",
    "queryMetaData": {
        "rows": 2,
        "errorCode": 0,
        "errorMessage": "OK",
        "sizeBytes" : 164
    },
    "columnMetaData": [
      {"name": "time", "type": "BIGINT"},
      {"name": "pid", "type": "INTEGER"},
      {"name": "mode", "type": "UNSIGNED BIGINT"},
      {"name": "path", "type": "TEXT"},
      {"name": "uid", "type": "INTEGER"},
     {"name": "lastAccess", "type": "TEXT"}
    ],
    "columnData": [
        [1518221620, 21306, 1000755, "/usr/bin/someexe", 500, "Tue Jan 14 15:53:13 2020 UTC"],
        [1518221620, 50331, 0, "/usr/bin/someOther.exe", null, "Tue Jan 14 15:53:13 2020 UTC"]
    ]
})";
    ResponseDispatcher dispatcher;
    std::string calculated = dispatcher.serializeToJson(response);
    EXPECT_TRUE(serializedJsonContentAreEquivalent(expected, calculated))<< "\nCalculated: "<< calculated;
}


TEST_F(TestResponseDispatcher, invalidNumbersWillResultInTheDataConvertedToZeroValue)
{
    ResponseData::ColumnData columnData;
    ResponseData::RowData  rowData;
    rowData["pathname"] = "anyfile";
    rowData["sophosPID"] = "17984:132164677472649892";
    rowData["start_time"] = "50330";
    columnData.push_back(rowData);

    // create a start_time that is not integer.
    rowData["start_time"] = "thisIsNotInteger";
    columnData.push_back(rowData);
    // third row
    rowData["start_time"] = "35980";
    columnData.push_back(rowData);

    QueryResponse response{ResponseStatus{ErrorCode::SUCCESS},
                           ResponseData{headerExample(), columnData},
                           ResponseMetaData()};

    std::string expected = R"({
"type": "sophos.mgt.response.RunLiveQuery",
"queryMetaData": {"errorCode":0,"errorMessage":"OK","rows":3,"sizeBytes":132},
"columnMetaData": [{"name":"pathname","type":"TEXT"},{"name":"sophosPID","type":"TEXT"},{"name":"start_time","type":"BIGINT"}],
"columnData": [["anyfile","17984:132164677472649892",50330],["anyfile","17984:132164677472649892",0],["anyfile","17984:132164677472649892",35980]]
}
)";

    ResponseDispatcher dispatcher;
    std::string calculated = dispatcher.serializeToJson(response);
    EXPECT_TRUE(serializedJsonContentAreEquivalent(expected, calculated))<< "\nCalculated: "<< calculated;
}

TEST_F(TestResponseDispatcher, emptyNumberShouldBeSentAsNull)
{
    ResponseData::ColumnData columnData;
    ResponseData::RowData  rowData;
    rowData["pathname"] = "anyfile";
    rowData["sophosPID"] = "17984:132164677472649892";
    rowData["start_time"] = "50330";
    columnData.push_back(rowData);

    // create a start_time that is empty
    rowData["start_time"] = "";
    columnData.push_back(rowData);
    // third row
    rowData["start_time"] = "35980";
    columnData.push_back(rowData);

    QueryResponse response{ResponseStatus{ErrorCode::SUCCESS},
                           ResponseData{headerExample(), columnData},
                           ResponseMetaData()};

    std::string expected = R"({
    "type": "sophos.mgt.response.RunLiveQuery",
    "queryMetaData": {
        "rows": 3,
        "errorCode": 0,
        "errorMessage": "OK",
        "sizeBytes": 135
    },
    "columnMetaData": [
      {"name": "pathname", "type": "TEXT"},
      {"name": "sophosPID", "type": "TEXT"},
      {"name": "start_time", "type": "BIGINT"}
    ],
    "columnData": [
        ["anyfile","17984:132164677472649892", 50330],
        ["anyfile","17984:132164677472649892", null],
        ["anyfile","17984:132164677472649892", 35980]
    ]
})";
    ResponseDispatcher dispatcher;
    std::string calculated = dispatcher.serializeToJson(response);
    EXPECT_TRUE(serializedJsonContentAreEquivalent(expected, calculated))
                        << "\nCalculated: "<< calculated << ".\n expected: \n" << expected;
}


TEST_F(TestResponseDispatcher, missingEntriesShouldBeSetToNull)
{
    ResponseData::ColumnData columnData;
    ResponseData::RowData  firstRow;
    firstRow["pathname"] = "anyfile";
    firstRow["sophosPID"] = "17984:132164677472649892";
    firstRow["start_time"] = "50330";
    columnData.push_back(firstRow);

    ResponseData::RowData  secondRow;
    secondRow["pathname"] = "anyfile";
    // do not have sophosPID
    //secondRow["sophosPID"] = "17984:132164677472649892";
    secondRow["start_time"] = "50330";
    columnData.push_back(secondRow);

    ResponseData::RowData  thirdRow;
    thirdRow["pathname"] = "anyfile";
    thirdRow["sophosPID"] = "17984:132164677472649892";
    //thirdRow["start_time"] = "50330";
    columnData.push_back(thirdRow);

    QueryResponse response{ResponseStatus{ErrorCode::SUCCESS},
                           ResponseData{headerExample(), columnData},
                           ResponseMetaData()};

    std::string expected = R"({
    "type": "sophos.mgt.response.RunLiveQuery",
    "queryMetaData": {
        "rows": 3,
        "errorCode": 0,
        "errorMessage": "OK",
        "sizeBytes" : 113
    },
    "columnMetaData": [
      {"name": "pathname", "type": "TEXT"},
      {"name": "sophosPID", "type": "TEXT"},
      {"name": "start_time", "type": "BIGINT"}
    ],
    "columnData": [
        ["anyfile","17984:132164677472649892", 50330],
        ["anyfile",null, 50330],
        ["anyfile","17984:132164677472649892", null]
    ]
})";
    ResponseDispatcher dispatcher;
    std::string calculated = dispatcher.serializeToJson(response);
    EXPECT_TRUE(serializedJsonContentAreEquivalent(expected, calculated))
                        << "\nCalculated: "<< calculated << ".\n expected: \n" << expected;
}

TEST_F(TestResponseDispatcher, JsonForExceededEntriesHasEmptyDataColumnsIfTheyExceed10Mb)
{
    ResponseData::ColumnData columnData;
    columnData.reserve(10*1024);
    ResponseData::RowData  firstRow;
    std::string largestring(1000,'b');
    firstRow["pathname"] = largestring;
    firstRow["sophosPID"] = "17984:132164677472649892";
    firstRow["start_time"] = "50330";
    for( long long i=0; i<10*1024; i++)
    {
        columnData.push_back(firstRow);
    }
    QueryResponse response{ResponseStatus{ErrorCode::SUCCESS},
                           ResponseData{headerExample(), columnData },
                           ResponseMetaData()};
    std::string expected = R"({
    "type": "sophos.mgt.response.RunLiveQuery",
    "queryMetaData": {
        "rows": 10240,
        "errorCode": 100,
        "errorMessage": "Response data exceeded 10MB",
        "sizeBytes" : 10629121
    },
    "columnMetaData": [
      {"name": "pathname", "type": "TEXT"},
      {"name": "sophosPID", "type": "TEXT"},
      {"name": "start_time", "type": "BIGINT"}
    ]
})";
    ResponseDispatcher dispatcher;
    std::string calculated = dispatcher.serializeToJson(response);
    EXPECT_TRUE(serializedJsonContentAreEquivalent(expected, calculated)) << calculated;
}

TEST_F(TestResponseDispatcher, JsonShouldContainDuration)
{
    QueryResponse response{
        ResponseStatus{ErrorCode::RESPONSEEXCEEDLIMIT},
        ResponseData{headerExample(), ResponseData::MarkDataExceeded::DataExceedLimit},
        ResponseMetaData(1)};

    std::string expected = R"({
    "type": "sophos.mgt.response.RunLiveQuery",
    "queryMetaData": {
        "durationMillis":100,
        "errorCode": 100,
        "errorMessage": "Response data exceeded 10MB",
        "sizeBytes" : 0
    },
    "columnMetaData": [
      {"name": "pathname", "type": "TEXT"},
      {"name": "sophosPID", "type": "TEXT"},
      {"name": "start_time", "type": "BIGINT"}
    ]
})";
    ResponseDispatcher dispatcher;
    std::string calculated = dispatcher.serializeToJson(response);

    nlohmann::json jCalc = nlohmann::json::parse(calculated);
    nlohmann::json jExp = nlohmann::json::parse(expected);

    long calculatedDuration = jCalc["queryMetaData"]["durationMillis"].get<long>();
    long expectedDuration = jExp["queryMetaData"]["durationMillis"].get<long>();

    EXPECT_GT(calculatedDuration,  expectedDuration);
    jCalc["queryMetaData"]["durationMillis"] = 0;
    jExp["queryMetaData"]["durationMillis"] = 0;
    EXPECT_TRUE(jCalc == jExp) << "The calculated JSON:" << jCalc.dump() << " does not match the expected JSON: " <<  jExp.dump() << " - Note that the ordering does not matter!";
}

namespace
{
    class ResposeDispatcherWithMockFileSystem : public LogOffInitializedTests
    {
    public:
        ResposeDispatcherWithMockFileSystem()
        {
            mockFileSystem = new ::testing::NiceMock<MockFileSystem>();
            mockFilePermissions = new ::testing::NiceMock<MockFilePermissions>();
            Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });
            Tests::replaceFilePermissions(
                std::unique_ptr<Common::FileSystem::IFilePermissions> { mockFilePermissions });
        }
        ~ResposeDispatcherWithMockFileSystem() override
        {
            Tests::restoreFileSystem();
        }
        MockFileSystem* mockFileSystem; // Borrowed pointer
        MockFilePermissions* mockFilePermissions;  // Borrowed pointer
    };
}

TEST_F(ResposeDispatcherWithMockFileSystem, sendResponseShouldCreateFileAsExpected)
{
    ResponseData::ColumnData columnData;
    ResponseData::RowData  rowData;
    rowData["pathname"] = R"(C:\\Windows\\System32\\pacjsworker.exe)";
    rowData["sophosPID"] = "17984:132164677472649892";
    rowData["start_time"] = "50330";
    columnData.push_back(rowData);
    // the only difference between first and second row is the start_time
    rowData["start_time"] = "50331";
    columnData.push_back(rowData);
    QueryResponse response{ResponseStatus{ErrorCode::SUCCESS},
                           ResponseData{headerExample(), columnData},
                           ResponseMetaData()};
    ResponseDispatcher dispatcher;

    std::string rootInstall = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();
    std::string expectedPath = rootInstall + "/base/mcs/response/LiveQuery_correlation_response.json";
    EXPECT_CALL(*mockFileSystem, moveFile(_, expectedPath));
    EXPECT_CALL(*mockFilePermissions, chown(_,"sophos-spl-user","sophos-spl-group"));
    dispatcher.sendResponse("correlation", response);
}
