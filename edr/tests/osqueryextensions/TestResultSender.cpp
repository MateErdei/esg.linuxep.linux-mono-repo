/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/Helpers/LogInitializedTests.h>
#include <Common/Helpers/MockFileSystem.h>
#include <modules/livequery/ResponseData.h>
#include <modules/osqueryextensions/ResultsSender.h>
#include <Common/Helpers/FileSystemReplaceAndRestore.h>

#include <gtest/gtest.h>

using namespace ::testing;
//
//livequery::ResponseData::ColumnHeaders headerStrIntStr()
//{
//    livequery::ResponseData::ColumnHeaders  headers;
//    headers.emplace_back("first", livequery::ResponseData::AcceptedTypes::STRING);
//    headers.emplace_back("second", livequery::ResponseData::AcceptedTypes::BIGINT);
//    headers.emplace_back("third", livequery::ResponseData::AcceptedTypes::STRING);
//    return headers;
//}
//
//livequery::ResponseData::RowData singleRowStrIntStr(int row)
//{
//    livequery::ResponseData::RowData rowData;
//    rowData["first"] = "firstvalue" + std::to_string(row);
//    rowData["second"] = std::to_string(row);
//    rowData["third"] = "thirdvalue" + std::to_string(row);
//    return rowData;
//}

class TestResultSender : public LogOffInitializedTests{};

//TEST_F(TestResultSender, emptyResponseDataShouldNotThrow) // NOLINT
//{
//    livequery::ResponseData emptyData{livequery::ResponseData::emptyResponse()};
//    EXPECT_FALSE(emptyData.hasDataExceededLimit());
//    EXPECT_FALSE(emptyData.hasHeaders());
//}

//
//TEST_F(TestResponseData, exceedDataLimit) // NOLINT
//{
//    livequery::ResponseData exceeded{headerStrIntStr(), livequery::ResponseData::MarkDataExceeded::DataExceedLimit};
//    EXPECT_TRUE(exceeded.hasDataExceededLimit());
//    EXPECT_TRUE(exceeded.hasHeaders());
//}
//
//TEST_F(TestResponseData, validDataShouldBeConstructedNormally) // NOLINT
//{
//    livequery::ResponseData::ColumnData columnData;
//    columnData.push_back( singleRowStrIntStr(1));
//    columnData.push_back(singleRowStrIntStr(2));
//
//    livequery::ResponseData::ColumnData  copyColumnData{columnData};
//
//    livequery::ResponseData responseData{headerStrIntStr(), columnData};
//    EXPECT_FALSE(responseData.hasDataExceededLimit());
//    EXPECT_TRUE(responseData.hasHeaders());
//    EXPECT_EQ( copyColumnData, responseData.columnData());
//    EXPECT_EQ( headerStrIntStr(), responseData.columnHeaders());
//}
//
//TEST_F(TestResponseData, valuesInsideTheCellsWillNotBeCheckedByResponseDataOnlyTheStructure) // NOLINT
//{
//    livequery::ResponseData::ColumnData columnData;
//    columnData.push_back( singleRowStrIntStr(1));
//    auto second = singleRowStrIntStr(2);
//    // although the value is not integer, this will not be checked directly by
//    // ResponseData, it will be dealt with on the generation of the Json file.
//    second["second"] = "notInteger";
//    columnData.push_back(second);
//
//    livequery::ResponseData::ColumnData  copyColumnData{columnData};
//
//    livequery::ResponseData responseData{headerStrIntStr(), columnData};
//    EXPECT_FALSE(responseData.hasDataExceededLimit());
//    EXPECT_TRUE(responseData.hasHeaders());
//    EXPECT_EQ( copyColumnData, responseData.columnData());
//    EXPECT_EQ( headerStrIntStr(), responseData.columnHeaders());
//}
//
//TEST_F(TestResponseData, rowsWithExtraElementsWillThrowException) // NOLINT
//{
//    livequery::ResponseData::ColumnData columnData;
//    columnData.push_back( singleRowStrIntStr(1));
//    auto second = singleRowStrIntStr(2);
//    second["forth"] = "anything";
//    columnData.push_back(second);
//
//    EXPECT_THROW(livequery::ResponseData(headerStrIntStr(), columnData), livequery::InvalidReponseData);
//}
//
//TEST_F(TestResponseData, rowsAreAllowedToHaveMissingElements) // NOLINT
//{
//    livequery::ResponseData::ColumnData columnData;
//    columnData.push_back( singleRowStrIntStr(1));
//    auto second = singleRowStrIntStr(2);
//    second.erase("second");
//    columnData.push_back(second);
//
//    EXPECT_NO_THROW(livequery::ResponseData(headerStrIntStr(), columnData));
//}
//
//TEST_F(TestResponseData, rowsWithDifferentElementsWillThrowException) // NOLINT
//{
//    livequery::ResponseData::ColumnData columnData;
//    columnData.push_back( singleRowStrIntStr(1));
//    auto second = singleRowStrIntStr(2);
//    second.erase("second");
//    second["forth"] = "anyvalue";
//    columnData.push_back(second);
//
//    EXPECT_THROW(livequery::ResponseData(headerStrIntStr(), columnData), livequery::InvalidReponseData);
//}


///////////////////////////////////

//TEST_F(TestResultSender, ResetRemovesExistingBatchFile) // NOLINT
//{
//    auto mockFileSystem = new ::testing::NiceMock<MockFileSystem>();
////    EXPECT_CALL(*mockFileSystem, isFile(_)).Times(2).WillOnce(Return(true));
////    EXPECT_CALL(*mockFileSystem, isFile(filepath)).WillOnce(Return(false));
////    EXPECT_CALL(*mockFileSystem, isFile("/etc/ssl/certs/ca-certificates.crt")).WillOnce(Return(false));
////    EXPECT_CALL(*mockFileSystem, isFile("/etc/pki/tls/certs/ca-bundle.crt")).WillOnce(Return(true));
////    EXPECT_CALL(*mockFileSystem, writeFile(filepath,
////                                           HasSubstr("--tls_server_certs=/etc/pki/tls/certs/ca-bundle.crt"))).WillOnce(
////        Invoke([&fileContent](const std::string&, const std::string& content) { fileContent = content; }));
//    std::string config = "{}";
//    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });
//    EXPECT_CALL(*mockFileSystem, exists("/opt/sophos-spl/plugins/edr/var/tmp_file")).WillOnce(Return(false));
//    EXPECT_CALL(*mockFileSystem, readFile("/opt/sophos-spl/plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf")).WillOnce(Return(config));
//    ResultsSender resultsSender;
////        ResultsSender resultsSender(
////        L"Path1",
////        L"Path2",
////        L"Path3",
////        mockFileHelper_,
////        mockMetricsFile_,
////        mockQueryPackMapper_,
////        mockDataUsageFileManager_);
//    EXPECT_CALL(*mockFileSystem, removeFile("/opt/sophos-spl/plugins/edr/var/tmp_file")).Times(1);
//    EXPECT_CALL(*mockFileSystem, appendFile("/opt/sophos-spl/plugins/edr/var/tmp_file", "[")).Times(1);
//    resultsSender.Reset();
//    EXPECT_CALL(*mockFileSystem, exists("/opt/sophos-spl/plugins/edr/var/tmp_file")).WillOnce(Return(false));
//}
//
//TEST_F(ResultsSenderTests, ResetPropogatesException)
//{
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(false));
//    ResultsSender resultsSender(
//        L"Path1",
//        L"Path2",
//        L"Path3",
//        mockFileHelper_,
//        mockMetricsFile_,
//        mockQueryPackMapper_,
//        mockDataUsageFileManager_);
//    EXPECT_CALL(mockFileHelper_, RemoveFile(StrEq(L"Path1"))).WillOnce(Throw(std::runtime_error("TEST")));
//    EXPECT_THROW(resultsSender.Reset(), std::runtime_error);
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(false));
//}
//
//TEST_F(ResultsSenderTests, AddWritesToFile)
//{
//    EXPECT_CALL(mockQueryPackMapper_, GetQueryTagMap()).Times(1);
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(false));
//    std::string testResult = "{\"name\":\"\",\"test\":\"value\"}";
//    EXPECT_CALL(mockDataUsageFileManager_, UpdateDataDailyUsage(testResult.size())).Times(1);
//    ResultsSender resultsSender(
//        L"Path1",
//        L"Path2",
//        L"Path3",
//        mockFileHelper_,
//        mockMetricsFile_,
//        mockQueryPackMapper_,
//        mockDataUsageFileManager_);
//    EXPECT_CALL(mockFileHelper_, Append(StrEq(L"Path1"), StrEq(testResult))).Times(1);
//    EXPECT_CALL(mockMetricsFile_, Add("", testResult.size()));
//
//    resultsSender.Add(testResult);
//
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(false));
//}
//
//TEST_F(ResultsSenderTests, AddAppendsToFileExistinEntries)
//{
//    EXPECT_CALL(mockQueryPackMapper_, GetQueryTagMap()).Times(2);
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(false));
//    ResultsSender resultsSender(
//        L"Path1",
//        L"Path2",
//        L"Path3",
//        mockFileHelper_,
//        mockMetricsFile_,
//        mockQueryPackMapper_,
//        mockDataUsageFileManager_);
//    std::string testResultString1 = "{\"name\":\"\",\"test\":\"value\"}";
//    std::string testResultString2 = "{\"name\":\"\",\"test2\":\"value2\"}";
//    EXPECT_CALL(mockDataUsageFileManager_, UpdateDataDailyUsage(testResultString1.size())).Times(1);
//    EXPECT_CALL(mockFileHelper_, Append(StrEq(L"Path1"), StrEq(testResultString1))).Times(1);
//    EXPECT_CALL(mockDataUsageFileManager_, UpdateDataDailyUsage(testResultString2.size())).Times(1);
//    EXPECT_CALL(mockFileHelper_, Append(StrEq(L"Path1"), StrEq("," + testResultString2))).Times(1);
//    EXPECT_CALL(mockMetricsFile_, Add("", testResultString1.size()));
//    EXPECT_CALL(mockMetricsFile_, Add("", testResultString2.size()));
//
//    resultsSender.Add(testResultString1);
//    resultsSender.Add(testResultString2);
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(false));
//}
//
//TEST_F(ResultsSenderTests, AddThrowsInvalidJsonLog)
//{
//    MockLogger mockLogger;
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(false));
//    ResultsSender resultsSender(
//        L"Path1",
//        L"Path2",
//        L"Path3",
//        mockFileHelper_,
//        mockMetricsFile_,
//        mockQueryPackMapper_,
//        mockDataUsageFileManager_);
//    EXPECT_CALL(mockFileHelper_, Append(_, _)).Times(0);
//    EXPECT_THROW(resultsSender.Add("{\"test\":\"value\""), std::exception);
//    ASSERT_THAT(
//        mockLogger.GetItems(), HasSubstr(L"Invalid JSON log message."));
//
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(false));
//}
//
//TEST_F(ResultsSenderTests, AddThrowsWhenAppendThrows)
//{
//    EXPECT_CALL(mockQueryPackMapper_, GetQueryTagMap()).Times(1);
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(false));
//    ResultsSender resultsSender(
//        L"Path1",
//        L"Path2",
//        L"Path3",
//        mockFileHelper_,
//        mockMetricsFile_,
//        mockQueryPackMapper_,
//        mockDataUsageFileManager_);
//    EXPECT_CALL(mockFileHelper_, Append(_, _)).WillOnce(Throw(std::runtime_error("TEST")));
//    EXPECT_THROW(resultsSender.Add("{\"test\":\"value\"}"), std::runtime_error);
//
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(false));
//}
//
//TEST_F(ResultsSenderTests, AddThrowsWhenUpdateDailyUsageThrows)
//{
//    EXPECT_CALL(mockQueryPackMapper_, GetQueryTagMap()).Times(1);
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(false));
//    std::string testResult = "{\"name\":\"\",\"test\":\"value\"}";
//    EXPECT_CALL(mockDataUsageFileManager_, UpdateDataDailyUsage(testResult.size()))
//        .WillOnce(Throw(std::runtime_error("TEST")));
//    ResultsSender resultsSender(
//        L"Path1",
//        L"Path2",
//        L"Path3",
//        mockFileHelper_,
//        mockMetricsFile_,
//        mockQueryPackMapper_,
//        mockDataUsageFileManager_);
//
//    EXPECT_CALL(mockFileHelper_, Append(StrEq(L"Path1"), StrEq(testResult))).Times(1);
//    EXPECT_CALL(mockMetricsFile_, Add(_, _)).Times(1);
//
//    EXPECT_THROW(resultsSender.Add(testResult), std::runtime_error);
//
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(false));
//}
//
//TEST_F(ResultsSenderTests, GetFileSizeQueriesFile)
//{
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(false)).WillOnce(Return(true));
//    ResultsSender resultsSender(
//        L"Path1",
//        L"Path2",
//        L"Path3",
//        mockFileHelper_,
//        mockMetricsFile_,
//        mockQueryPackMapper_,
//        mockDataUsageFileManager_);
//    EXPECT_CALL(mockFileHelper_, Size(StrEq(L"Path1"))).WillOnce(Return(1));
//    ASSERT_EQ(resultsSender.GetFileSize(), 3);
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(false));
//}
//
//TEST_F(ResultsSenderTests, GetFileSizeZeroWhenFileDoesNotExist)
//{
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(false));
//    ResultsSender resultsSender(
//        L"Path1",
//        L"Path2",
//        L"Path3",
//        mockFileHelper_,
//        mockMetricsFile_,
//        mockQueryPackMapper_,
//        mockDataUsageFileManager_);
//
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(false));
//    EXPECT_CALL(mockFileHelper_, Size(StrEq(L"Path1"))).Times(0);
//    ASSERT_EQ(resultsSender.GetFileSize(), 0);
//
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(false));
//}
//
//TEST_F(ResultsSenderTests, GetFileSizePropagatesException)
//{
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(false)).WillOnce(Return(true));
//    ResultsSender resultsSender(
//        L"Path1",
//        L"Path2",
//        L"Path3",
//        mockFileHelper_,
//        mockMetricsFile_,
//        mockQueryPackMapper_,
//        mockDataUsageFileManager_);
//    EXPECT_CALL(mockFileHelper_, Size(StrEq(L"Path1"))).WillOnce(Throw(std::runtime_error("TEST")));
//    EXPECT_THROW(resultsSender.GetFileSize(), std::runtime_error);
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(false));
//}
//
//TEST_F(ResultsSenderTests, SendMovesBatchFile)
//{
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(false));
//    ResultsSender resultsSender(
//        L"Path1",
//        L"Path2",
//        L"Path3",
//        mockFileHelper_,
//        mockMetricsFile_,
//        mockQueryPackMapper_,
//        mockDataUsageFileManager_);
//
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(true));
//    EXPECT_CALL(mockDataUsageFileManager_, IsDailyDataLimitHit()).WillOnce(Return(false));
//    EXPECT_CALL(mockFileHelper_, Append(StrEq(L"Path1"), StrEq("]"))).Times(1);
//    EXPECT_CALL(mockFileHelper_, FileMove(StrEq(L"Path1"), StartsWith(L"Path2\\scheduled-"), false)).Times(1);
//    EXPECT_CALL(mockMetricsFile_, ClearExpired()).Times(1);
//
//    resultsSender.Send();
//
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(false));
//}
//
//TEST_F(ResultsSenderTests, SendWithTrailsCopiesAndMovesBatchFile)
//{
//    EXPECT_CALL(mockFileHelper_, CreateFolder(StrEq(L"Trail")));
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(false));
//    ResultsSender resultsSender(
//        L"Path1",
//        L"Path2",
//        L"Trail",
//        mockFileHelper_,
//        mockMetricsFile_,
//        mockQueryPackMapper_,
//        mockDataUsageFileManager_,
//        true);
//
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(true));
//    EXPECT_CALL(mockFileHelper_, Append(StrEq(L"Path1"), StrEq("]"))).Times(1);
//    EXPECT_CALL(mockFileHelper_, FileCopy(StrEq(L"Path1"), StartsWith(L"Trail\\scheduled-"))).Times(1);
//    EXPECT_CALL(mockFileHelper_, FileMove(StrEq(L"Path1"), StartsWith(L"Path2\\scheduled-"), false)).Times(1);
//    EXPECT_CALL(mockDataUsageFileManager_, IsDailyDataLimitHit()).WillOnce(Return(false));
//    EXPECT_CALL(mockMetricsFile_, ClearExpired()).Times(1);
//
//    resultsSender.Send();
//
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(false));
//}
//
//TEST_F(ResultsSenderTests, SendWithTrailsCopiesFailureStillMovesBatchFile)
//{
//    EXPECT_CALL(mockFileHelper_, CreateFolder(StrEq(L"Trail")));
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(false));
//    ResultsSender resultsSender(
//        L"Path1",
//        L"Path2",
//        L"Trail",
//        mockFileHelper_,
//        mockMetricsFile_,
//        mockQueryPackMapper_,
//        mockDataUsageFileManager_,
//        true);
//
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(true));
//    EXPECT_CALL(mockFileHelper_, Append(StrEq(L"Path1"), StrEq("]"))).Times(1);
//    EXPECT_CALL(mockFileHelper_, FileCopy(StrEq(L"Path1"), StartsWith(L"Trail\\scheduled-")))
//        .WillOnce(Throw(std::runtime_error("FAIL")));
//    EXPECT_CALL(mockFileHelper_, FileMove(StrEq(L"Path1"), StartsWith(L"Path2\\scheduled-"), false)).Times(1);
//    EXPECT_CALL(mockDataUsageFileManager_, IsDailyDataLimitHit()).WillOnce(Return(false));
//    EXPECT_CALL(mockMetricsFile_, ClearExpired()).Times(1);
//
//    resultsSender.Send();
//
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(false));
//}
//
//TEST_F(ResultsSenderTests, SendDoesNotMoveNoBatchFile)
//{
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(false));
//    ResultsSender resultsSender(
//        L"Path1",
//        L"Path2",
//        L"Path3",
//        mockFileHelper_,
//        mockMetricsFile_,
//        mockQueryPackMapper_,
//        mockDataUsageFileManager_);
//
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(false));
//    EXPECT_CALL(mockFileHelper_, FileMove(_, _, _)).Times(0);
//
//    resultsSender.Send();
//
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(false));
//}
//
//TEST_F(ResultsSenderTests, SendThrowsWhenFileMoveThrows)
//{
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(false));
//    ResultsSender resultsSender(
//        L"Path1",
//        L"Path2",
//        L"Path3",
//        mockFileHelper_,
//        mockMetricsFile_,
//        mockQueryPackMapper_,
//        mockDataUsageFileManager_);
//
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(true));
//    EXPECT_CALL(mockFileHelper_, Append(StrEq(L"Path1"), StrEq("]"))).Times(1);
//    EXPECT_CALL(mockFileHelper_, FileMove(StrEq(L"Path1"), StartsWith(L"Path2\\scheduled-"), false))
//        .WillOnce(Throw(std::runtime_error("TEST")));
//
//    EXPECT_THROW(resultsSender.Send(), std::runtime_error);
//
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(false));
//}
//
//TEST_F(ResultsSenderTests, SendIfFileExistsAtStart)
//{
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(true));
//    EXPECT_CALL(mockDataUsageFileManager_, IsDailyDataLimitHit()).WillOnce(Return(false));
//    EXPECT_CALL(mockFileHelper_, Append(StrEq(L"Path1"), StrEq("]"))).Times(1);
//    EXPECT_CALL(mockFileHelper_, FileMove(StrEq(L"Path1"), StartsWith(L"Path2\\scheduled-"), false)).Times(1);
//    EXPECT_CALL(mockMetricsFile_, ClearExpired()).Times(1);
//    ResultsSender resultsSender(
//        L"Path1",
//        L"Path2",
//        L"Path3",
//        mockFileHelper_,
//        mockMetricsFile_,
//        mockQueryPackMapper_,
//        mockDataUsageFileManager_);
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(false));
//}
//
//TEST_F(ResultsSenderTests, SendIfFileExistsAtShutdown)
//{
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(false));
//    EXPECT_CALL(mockDataUsageFileManager_, IsDailyDataLimitHit()).WillOnce(Return(false));
//    ResultsSender resultsSender(
//        L"Path1",
//        L"Path2",
//        L"Path3",
//        mockFileHelper_,
//        mockMetricsFile_,
//        mockQueryPackMapper_,
//        mockDataUsageFileManager_);
//
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(true));
//    EXPECT_CALL(mockFileHelper_, Append(StrEq(L"Path1"), StrEq("]"))).Times(1);
//    EXPECT_CALL(mockFileHelper_, FileMove(StrEq(L"Path1"), StartsWith(L"Path2\\scheduled-"), false)).Times(1);
//    EXPECT_CALL(mockMetricsFile_, ClearExpired()).Times(1);
//}
//
//TEST_F(ResultsSenderTests, SendThrowsIfDataUsageManagerThrows)
//{
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(false));
//    ResultsSender resultsSender(
//        L"Path1",
//        L"Path2",
//        L"Path3",
//        mockFileHelper_,
//        mockMetricsFile_,
//        mockQueryPackMapper_,
//        mockDataUsageFileManager_);
//
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(true));
//    EXPECT_CALL(mockFileHelper_, Append(StrEq(L"Path1"), StrEq("]"))).Times(1);
//    EXPECT_CALL(mockFileHelper_, FileMove(StrEq(L"Path1"), StartsWith(L"Path2\\scheduled-"), false)).Times(1);
//    EXPECT_CALL(mockDataUsageFileManager_, IsDailyDataLimitHit()).WillOnce(Throw(std::runtime_error("TEST")));
//
//    EXPECT_THROW(resultsSender.Send(), std::runtime_error);
//
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(false));
//}
//
//TEST_F(ResultsSenderTests, FirstAddFailureDoesNotAddCommaNext)
//{
//    EXPECT_CALL(mockQueryPackMapper_, GetQueryTagMap()).Times(2);
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(false));
//    ResultsSender resultsSender(
//        L"Path1",
//        L"Path2",
//        L"Path3",
//        mockFileHelper_,
//        mockMetricsFile_,
//        mockQueryPackMapper_,
//        mockDataUsageFileManager_);
//    std::string testJsonResult = "{\"name\":\"\",\"test\":\"value\"}";
//    EXPECT_CALL(mockFileHelper_, Append(StrEq(L"Path1"), StrEq(testJsonResult)))
//        .Times(2)
//        .WillOnce(Throw(std::runtime_error("TEST")))
//        .WillOnce(Return());
//    EXPECT_CALL(mockDataUsageFileManager_, UpdateDataDailyUsage(_)).Times(1);
//    EXPECT_CALL(mockMetricsFile_, Add("", testJsonResult.size()));
//    EXPECT_THROW(resultsSender.Add(testJsonResult), std::runtime_error);
//    resultsSender.Add(testJsonResult);
//
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(false));
//}
//
//TEST_F(ResultsSenderTests, TestTagAddedFromQueryPackMap)
//{
//    QueryPackTagsAndNames testQueryTagMap;
//    auto correctNameTagPair = std::make_pair<std::string, std::string>("test_query", "test_tag");
//    testQueryTagMap.insert(
//        std::pair<std::string, std::pair<std::string, std::string>>("test_query", correctNameTagPair));
//    std::string testResult = "{\"name\":\"test_query\"}";
//    std::string tagAppendedResult = "{\"name\":\"test_query\",\"tag\":\"test_tag\"}";
//    EXPECT_CALL(mockDataUsageFileManager_, UpdateDataDailyUsage(tagAppendedResult.size())).Times(1);
//    EXPECT_CALL(mockQueryPackMapper_, GetQueryTagMap()).WillOnce(Return(testQueryTagMap));
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(false));
//    EXPECT_CALL(mockMetricsFile_, Add("test_query", tagAppendedResult.size()));
//    ResultsSender resultsSender(
//        L"Path1",
//        L"Path2",
//        L"Path3",
//        mockFileHelper_,
//        mockMetricsFile_,
//        mockQueryPackMapper_,
//        mockDataUsageFileManager_);
//    EXPECT_CALL(mockFileHelper_, Append(StrEq(L"Path1"), StrEq(tagAppendedResult))).Times(1);
//    resultsSender.Add(testResult);
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(false));
//}
//
//TEST_F(ResultsSenderTests, TestQueryNameCorrectedFromQueryPackMap)
//{
//    QueryPackTagsAndNames testQueryTagMap;
//    auto correctNameTagPair = std::make_pair<std::string, std::string>("test_query", "test_tag");
//    testQueryTagMap.insert(
//        std::pair<std::string, std::pair<std::string, std::string>>("pack_testpack_test_query", correctNameTagPair));
//    std::string testResult = "{\"name\":\"pack_testpack_test_query\"}";
//    std::string correctedNameResult = "{\"name\":\"test_query\",\"tag\":\"test_tag\"}";
//    EXPECT_CALL(mockDataUsageFileManager_, UpdateDataDailyUsage(correctedNameResult.size())).Times(1);
//    EXPECT_CALL(mockQueryPackMapper_, GetQueryTagMap()).WillOnce(Return(testQueryTagMap));
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(false));
//    EXPECT_CALL(mockMetricsFile_, Add("test_query", correctedNameResult.size()));
//    ResultsSender resultsSender(
//        L"Path1",
//        L"Path2",
//        L"Path3",
//        mockFileHelper_,
//        mockMetricsFile_,
//        mockQueryPackMapper_,
//        mockDataUsageFileManager_);
//    EXPECT_CALL(mockFileHelper_, Append(StrEq(L"Path1"), StrEq(correctedNameResult))).Times(1);
//    resultsSender.Add(testResult);
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(false));
//}
//
//TEST_F(ResultsSenderTests, TestQueryMissingFromTagMap)
//{
//    QueryPackTagsAndNames testQueryTagMap;
//    std::string testResult = "{\"name\":\"test_query\"}";
//    EXPECT_CALL(mockDataUsageFileManager_, UpdateDataDailyUsage(testResult.size())).Times(1);
//    EXPECT_CALL(mockQueryPackMapper_, GetQueryTagMap()).WillOnce(Return(testQueryTagMap));
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(false));
//    EXPECT_CALL(mockMetricsFile_, Add("test_query", testResult.size()));
//    ResultsSender resultsSender(
//        L"Path1",
//        L"Path2",
//        L"Path3",
//        mockFileHelper_,
//        mockMetricsFile_,
//        mockQueryPackMapper_,
//        mockDataUsageFileManager_);
//    EXPECT_CALL(mockFileHelper_, Append(StrEq(L"Path1"), StrEq(testResult))).Times(1);
//    resultsSender.Add(testResult);
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(false));
//}
//
//TEST_F(ResultsSenderTests, SendWithLimitNotMet)
//{
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(true));
//    EXPECT_CALL(mockFileHelper_, Append(StrEq(L"Path1"), StrEq("]"))).Times(1);
//    EXPECT_CALL(mockFileHelper_, FileMove(StrEq(L"Path1"), StartsWith(L"Path2\\scheduled-"), false)).Times(1);
//    EXPECT_CALL(mockDataUsageFileManager_, IsDailyDataLimitHit()).WillOnce(Return(false));
//    EXPECT_CALL(mockMetricsFile_, ClearExpired()).Times(1);
//    ResultsSender resultsSender(
//        L"Path1",
//        L"Path2",
//        L"Path3",
//        mockFileHelper_,
//        mockMetricsFile_,
//        mockQueryPackMapper_,
//        mockDataUsageFileManager_);
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(false));
//}
//
//TEST_F(ResultsSenderTests, SendWithLimitMetReloads)
//{
//    MockLogger mockLogger;
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(true));
//    EXPECT_CALL(mockFileHelper_, Append(StrEq(L"Path1"), StrEq("]"))).Times(1);
//    EXPECT_CALL(mockFileHelper_, FileMove(StrEq(L"Path1"), StartsWith(L"Path2\\scheduled-"), false)).Times(1);
//    EXPECT_CALL(mockDataUsageFileManager_, IsDailyDataLimitHit()).WillOnce(Return(true));
//    EXPECT_CALL(mockDataUsageFileManager_, Reload()).Times(1);
//    EXPECT_CALL(mockDataUsageFileManager_, ScheduledDailyDataLimitBytes()).WillOnce(Return(100));
//    EXPECT_CALL(mockMetricsFile_, ClearExpired()).Times(1);
//    ResultsSender resultsSender(
//        L"Path1",
//        L"Path2",
//        L"Path3",
//        mockFileHelper_,
//        mockMetricsFile_,
//        mockQueryPackMapper_,
//        mockDataUsageFileManager_);
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(false));
//    ASSERT_THAT(
//        mockLogger.GetItems(), HasSubstr(L"The maximum daily scheduled query data limit 100 bytes has been reached."));
//}
//
//TEST_F(ResultsSenderTests, InitialExistsThrowsExceptionContinuesConstructor)
//{
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1")))
//        .WillOnce(Throw(std::runtime_error("TEST")))
//        .WillOnce(Return(false));
//    ResultsSender resultsSender(
//        L"Path1",
//        L"Path2",
//        L"Path3",
//        mockFileHelper_,
//        mockMetricsFile_,
//        mockQueryPackMapper_,
//        mockDataUsageFileManager_);
//}
//
//TEST_F(ResultsSenderTests, FailsToAddMetricsOnAdd)
//{
//    MockLogger mockLogger;
//    EXPECT_CALL(mockQueryPackMapper_, GetQueryTagMap());
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(false));
//    EXPECT_CALL(mockDataUsageFileManager_, UpdateDataDailyUsage(_));
//    ResultsSender resultsSender(
//        L"Path1",
//        L"Path2",
//        L"Path3",
//        mockFileHelper_,
//        mockMetricsFile_,
//        mockQueryPackMapper_,
//        mockDataUsageFileManager_);
//    EXPECT_CALL(mockFileHelper_, Append(StrEq(L"Path1"), _));
//    EXPECT_CALL(mockMetricsFile_, Add("", _)).WillOnce(Throw(std::runtime_error("FAIL")));
//
//    resultsSender.Add("{}");
//
//    EXPECT_CALL(mockFileHelper_, Exists(StrEq(L"Path1"))).WillOnce(Return(false));
//    ASSERT_THAT(mockLogger.GetItems(), HasSubstr(L"Failed to add query metrics: FAIL"));
//}
//
//TEST_F(ResultsSenderTests, FailsToClearMetricsOnSend)
//{
//    MockLogger mockLogger;
//    EXPECT_CALL(mockFileHelper_, Exists(_)).WillOnce(Return(true));
//    EXPECT_CALL(mockFileHelper_, Append(_, _));
//    EXPECT_CALL(mockFileHelper_, FileMove(_, _, false));
//    EXPECT_CALL(mockDataUsageFileManager_, IsDailyDataLimitHit()).WillOnce(Return(false));
//    EXPECT_CALL(mockMetricsFile_, ClearExpired()).WillOnce(Throw(std::runtime_error("FAIL")));
//    ResultsSender resultsSender(
//        L"Path1",
//        L"Path2",
//        L"Path3",
//        mockFileHelper_,
//        mockMetricsFile_,
//        mockQueryPackMapper_,
//        mockDataUsageFileManager_);
//    EXPECT_CALL(mockFileHelper_, Exists(_)).WillOnce(Return(false));
//    ASSERT_THAT(mockLogger.GetItems(), HasSubstr(L"Failed to clear expired metrics data: FAIL"));
//}