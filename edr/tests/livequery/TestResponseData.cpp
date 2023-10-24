// Copyright 2019-2023 Sophos Limited. All rights reserved.

#include "livequery/ResponseData.h"

#ifdef SPL_BAZEL
#include "tests/Common/Helpers/LogInitializedTests.h"
#else
#include "Common/Helpers/LogInitializedTests.h"
#endif

#include <gtest/gtest.h>

using namespace ::testing;

livequery::ResponseData::ColumnHeaders headerStrIntStr()
{
    livequery::ResponseData::ColumnHeaders  headers;
    headers.emplace_back("first", OsquerySDK::ColumnType::TEXT_TYPE);
    headers.emplace_back("second", OsquerySDK::ColumnType::BIGINT_TYPE);
    headers.emplace_back("third", OsquerySDK::ColumnType::TEXT_TYPE);
    return headers;
}

livequery::ResponseData::RowData singleRowStrIntStr(int row)
{
    livequery::ResponseData::RowData rowData;
    rowData["first"] = "firstvalue" + std::to_string(row);
    rowData["second"] = std::to_string(row);
    rowData["third"] = "thirdvalue" + std::to_string(row);
    return rowData;
}

class TestResponseData : public LogOffInitializedTests{};

TEST_F(TestResponseData, emptyResponseDataShouldNotThrow) // NOLINT
{
    livequery::ResponseData emptyData{livequery::ResponseData::emptyResponse()};
    EXPECT_FALSE(emptyData.hasDataExceededLimit());
    EXPECT_FALSE(emptyData.hasHeaders());
}

TEST_F(TestResponseData, exceedDataLimit) // NOLINT
{
    livequery::ResponseData exceeded{headerStrIntStr(), livequery::ResponseData::MarkDataExceeded::DataExceedLimit};
    EXPECT_TRUE(exceeded.hasDataExceededLimit());
    EXPECT_TRUE(exceeded.hasHeaders());
}

TEST_F(TestResponseData, validDataShouldBeConstructedNormally) // NOLINT
{
    livequery::ResponseData::ColumnData columnData;
    columnData.push_back( singleRowStrIntStr(1));
    columnData.push_back(singleRowStrIntStr(2));

    livequery::ResponseData::ColumnData  copyColumnData{columnData};

    livequery::ResponseData responseData{headerStrIntStr(), columnData};
    EXPECT_FALSE(responseData.hasDataExceededLimit());
    EXPECT_TRUE(responseData.hasHeaders());
    EXPECT_EQ( copyColumnData, responseData.columnData());
    EXPECT_EQ( headerStrIntStr(), responseData.columnHeaders());
}

TEST_F(TestResponseData, valuesInsideTheCellsWillNotBeCheckedByResponseDataOnlyTheStructure) // NOLINT
{
    livequery::ResponseData::ColumnData columnData;
    columnData.push_back( singleRowStrIntStr(1));
    auto second = singleRowStrIntStr(2);
    // although the value is not integer, this will not be checked directly by
    // ResponseData, it will be dealt with on the generation of the Json file.
    second["second"] = "notInteger";
    columnData.push_back(second);

    livequery::ResponseData::ColumnData  copyColumnData{columnData};

    livequery::ResponseData responseData{headerStrIntStr(), columnData};
    EXPECT_FALSE(responseData.hasDataExceededLimit());
    EXPECT_TRUE(responseData.hasHeaders());
    EXPECT_EQ( copyColumnData, responseData.columnData());
    EXPECT_EQ( headerStrIntStr(), responseData.columnHeaders());
}

TEST_F(TestResponseData, rowsWithExtraElementsWillThrowException) // NOLINT
{
    livequery::ResponseData::ColumnData columnData;
    columnData.push_back( singleRowStrIntStr(1));
    auto second = singleRowStrIntStr(2);
    second["forth"] = "anything";
    columnData.push_back(second);

    EXPECT_THROW(livequery::ResponseData(headerStrIntStr(), columnData), livequery::InvalidReponseData);
}

TEST_F(TestResponseData, rowsAreAllowedToHaveMissingElements) // NOLINT
{
    livequery::ResponseData::ColumnData columnData;
    columnData.push_back( singleRowStrIntStr(1));
    auto second = singleRowStrIntStr(2);
    second.erase("second");
    columnData.push_back(second);

    EXPECT_NO_THROW(livequery::ResponseData(headerStrIntStr(), columnData));
}

TEST_F(TestResponseData, rowsWithDifferentElementsWillThrowException) // NOLINT
{
    livequery::ResponseData::ColumnData columnData;
    columnData.push_back( singleRowStrIntStr(1));
    auto second = singleRowStrIntStr(2);
    second.erase("second");
    second["forth"] = "anyvalue";
    columnData.push_back(second);

    EXPECT_THROW(livequery::ResponseData(headerStrIntStr(), columnData), livequery::InvalidReponseData);
}

