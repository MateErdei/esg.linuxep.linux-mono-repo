/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>
#include <tests/googletest/googlemock/include/gmock/gmock-matchers.h>
#include <modules/livequery/ResponseDispatcher.h>
#include <thirdparty/nlohmann-json/json.hpp>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/Helpers/FileSystemReplaceAndRestore.h>
#include <Common/Helpers/MockFileSystem.h>
#include <Common/Helpers/MockFilePermissions.h>

using namespace ::testing;
using namespace livequery;

ResponseData::ColumnHeaders headerExample()
{
    ResponseData::ColumnHeaders  headers;
    headers.emplace_back("pathname", ResponseData::AcceptedTypes::STRING);
    headers.emplace_back("sophosPID", ResponseData::AcceptedTypes::STRING);
    headers.emplace_back("start_time", ResponseData::AcceptedTypes::BIGINT);
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
    return jv1 == jv2;
}

TEST(TestResponseDispatcher, verifySerializedJsonAreEquivalent)
{
    std::string v1 = R"({"data":{"a":"a","b":1}})";
    std::string v2 = R"({"data":{"b":1,"a":"a"}})";
    std::string v3 = R"({"data":{"b":"1","a":"a"}})";
    EXPECT_TRUE (serializedJsonContentAreEquivalent(v1,v2));
    EXPECT_FALSE (serializedJsonContentAreEquivalent(v1,v3));
    EXPECT_FALSE (serializedJsonContentAreEquivalent(v2,v3));
}

TEST(TestResponseDispatcher, JsonForExceededEntriesShouldNotIncludeDataColumns)
{
    QueryResponse response{ResponseStatus{ErrorCode::RESPONSEEXCEEDLIMIT},
                                      ResponseData{headerExample(), ResponseData::MarkDataExceeded::DataExceedLimit}};
    std::string expected = R"({
    "type": "sophos.mgt.response.RunLiveQuery",
    "queryMetaData": {
        "errorCode": 100,
        "errorMessage": "Response data exceeded 10MB"
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

TEST(TestResponseDispatcher, SpecificErrorConditionShouldBeIncludedInTheJsonFile)
{
    ResponseStatus status{ErrorCode::OSQUERYERROR};
    status.overrideErrorDescription("SpecialError condition reported by osquery");
    QueryResponse response{status,
                           ResponseData{headerExample(), ResponseData::ColumnData{}}};
    std::string expected = R"({
    "type": "sophos.mgt.response.RunLiveQuery",
    "queryMetaData": {
        "errorCode": 1,
        "errorMessage": "SpecialError condition reported by osquery"
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


TEST(TestResponseDispatcher, emptyResponseWhereNotRowWasSelectedShouldReturnExpectedJson)
{
    QueryResponse response{ResponseStatus{ErrorCode::SUCCESS},
                           ResponseData{headerExample(), ResponseData::ColumnData{}}};
    std::string expected = R"({
    "type": "sophos.mgt.response.RunLiveQuery",
    "queryMetaData": {
        "rows": 0,
        "errorCode": 0,
        "errorMessage": "OK"
    },
    "columnMetaData": [
      {"name": "pathname", "type": "TEXT"},
      {"name": "sophosPID", "type": "TEXT"},
      {"name": "start_time", "type": "BIGINT"}
    ]
})";
    ResponseDispatcher dispatcher;
    std::string calculated = dispatcher.serializeToJson(response);
    EXPECT_TRUE(serializedJsonContentAreEquivalent(expected, calculated))<< "\nCalculated: "<< calculated;
}

TEST(TestResponseDispatcher, validQueryResponseShouldReturnExpectedJson)
{
    ResponseData::ColumnData columnData;
    ResponseData::RowData  rowData;
    rowData["pathname"] = "C:\\Windows\\System32\\pacjsworker.exe";
    rowData["sophosPID"] = "17984:132164677472649892";
    rowData["start_time"] = "50330";
    columnData.push_back(rowData);
    // the only difference between first and second row is the start_time
    rowData["start_time"] = "50331";
    columnData.push_back(rowData);


    QueryResponse response{ResponseStatus{ErrorCode::SUCCESS},
                           ResponseData{headerExample(), columnData}};
    std::string expected = R"({
    "type": "sophos.mgt.response.RunLiveQuery",
    "queryMetaData": {
        "rows": 2,
        "errorCode": 0,
        "errorMessage": "OK"
    },
    "columnMetaData": [
      {"name": "pathname", "type": "TEXT"},
      {"name": "sophosPID", "type": "TEXT"},
      {"name": "start_time", "type": "BIGINT"}
    ],
    "columnData": [
        ["C:\\Windows\\System32\\pacjsworker.exe","17984:132164677472649892", 50330],
        ["C:\\Windows\\System32\\pacjsworker.exe","17984:132164677472649892", 50331]
    ]
})";
    ResponseDispatcher dispatcher;
    std::string calculated = dispatcher.serializeToJson(response);
    EXPECT_TRUE(serializedJsonContentAreEquivalent(expected, calculated))<< "\nCalculated: "<< calculated;
}


TEST(TestResponseDispatcher, invalidNumbersWillKeepTheirStringValue)
{
    ResponseData::ColumnData columnData;
    ResponseData::RowData  rowData;
    rowData["pathname"] = "anyfile";
    rowData["sophosPID"] = "17984:132164677472649892";
    rowData["start_time"] = "50330";
    columnData.push_back(rowData);

    // create a start_time that is not integer
    rowData["start_time"] = "thisIsNotInteger";
    columnData.push_back(rowData);
    // third row
    rowData["start_time"] = "35980";
    columnData.push_back(rowData);

    QueryResponse response{ResponseStatus{ErrorCode::SUCCESS},
                           ResponseData{headerExample(), columnData}};
    std::string expected = R"({
    "type": "sophos.mgt.response.RunLiveQuery",
    "queryMetaData": {
        "rows": 3,
        "errorCode": 0,
        "errorMessage": "OK"
    },
    "columnMetaData": [
      {"name": "pathname", "type": "TEXT"},
      {"name": "sophosPID", "type": "TEXT"},
      {"name": "start_time", "type": "BIGINT"}
    ],
    "columnData": [
        ["anyfile","17984:132164677472649892", 50330],
        ["anyfile","17984:132164677472649892", "thisIsNotInteger"],
        ["anyfile","17984:132164677472649892", 35980]
    ]
})";
    ResponseDispatcher dispatcher;
    std::string calculated = dispatcher.serializeToJson(response);
    EXPECT_TRUE(serializedJsonContentAreEquivalent(expected, calculated))<< "\nCalculated: "<< calculated;
}



class ResposeDispatcherWithMockFileSystem: public ::testing::Test
{
public:
    ResposeDispatcherWithMockFileSystem()
    {
        mockFileSystem = new ::testing::NiceMock<MockFileSystem>();
        mockFilePermissions = new ::testing::NiceMock<MockFilePermissions>();
        Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{mockFileSystem});
        Tests::replaceFilePermissions(std::unique_ptr<Common::FileSystem::IFilePermissions>{mockFilePermissions});
    }
    ~ResposeDispatcherWithMockFileSystem()
    {
        Tests::restoreFileSystem();
    }
    MockFileSystem * mockFileSystem;
    MockFilePermissions * mockFilePermissions;
};

TEST_F(ResposeDispatcherWithMockFileSystem, sendResponseShouldCreateFileAsExpected)
{

    ResponseData::ColumnData columnData;
    ResponseData::RowData  rowData;
    rowData["pathname"] = "C:\\\\Windows\\\\System32\\\\pacjsworker.exe";
    rowData["sophosPID"] = "17984:132164677472649892";
    rowData["start_time"] = "50330";
    columnData.push_back(rowData);
    // the only difference between first and second row is the start_time
    rowData["start_time"] = "50331";
    columnData.push_back(rowData);
    QueryResponse response{ResponseStatus{ErrorCode::SUCCESS},
                           ResponseData{headerExample(), columnData}};
    ResponseDispatcher dispatcher;
    EXPECT_CALL(*mockFileSystem, moveFile(_,"/opt/sophos-spl/base/mcs/response/LiveQuery_correlation_response.json"));
    EXPECT_CALL(*mockFilePermissions, chown(_,"sophos-spl-user","sophos-spl-group"));
    dispatcher.sendResponse("correlation", response);
}


