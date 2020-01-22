/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "MockOsqueryClient.h"

#include <modules/osqueryclient/OsqueryProcessor.h>
#include <thrift/transport/TTransportException.h>

#include <gtest/gtest.h>

using namespace ::testing;

class TestOSQueryProcessorWithMock : public ::testing::Test
{
public:
    std::unique_ptr<MockIOsqueryClient> m_mockClientPtr;
    std::string m_testFakeSocketPath;

    TestOSQueryProcessorWithMock() :
        m_mockClientPtr(new MockIOsqueryClient()),
        m_testFakeSocketPath(("/fake/osquery.sock"))
    {
    }

    void setupMockClient()
    {
        m_mockClientPtr = std::make_unique<MockIOsqueryClient>();
        osqueryclient::factory().replace([this]() { return std::move(m_mockClientPtr); });
    }

    ::testing::AssertionResult responseIsEquivalent(
        const char* m_expr,
        const char* n_expr,
        const livequery::QueryResponse& actualResponse,
        const livequery::QueryResponse& expectedResponse);

    ~TestOSQueryProcessorWithMock()
    {
        osqueryclient::factory().restore();
    }

    livequery::QueryResponse success(
        livequery::ResponseData::ColumnHeaders headers,
        livequery::ResponseData::ColumnData columnData)
    {
        livequery::ResponseStatus status { livequery::ErrorCode::SUCCESS };

        livequery::QueryResponse response { status, livequery::ResponseData { headers, columnData } };

        livequery::ResponseMetaData metaData;
        response.setMetaData(metaData);
        return response;
    }

    livequery::QueryResponse failure(livequery::ErrorCode errorCode, const std::string& description)
    {
        livequery::ResponseStatus status { errorCode };
        status.overrideErrorDescription(description);

        livequery::QueryResponse response { status, livequery::ResponseData::emptyResponse() };

        livequery::ResponseMetaData metaData;
        response.setMetaData(metaData);
        return response;
    }
};

::testing::AssertionResult TestOSQueryProcessorWithMock::responseIsEquivalent(
    const char* m_expr,
    const char* n_expr,
    const livequery::QueryResponse& actualResponse,
    const livequery::QueryResponse& expectedResponse)
{
    std::stringstream s;
    s << m_expr << " and " << n_expr << " failed: ";
    if (expectedResponse.status().errorCode() != actualResponse.status().errorCode())
    {
        return ::testing::AssertionFailure()
               << s.str() << " Expected response status : " << expectedResponse.status().errorValue()
               << ". Actual response status: " << actualResponse.status().errorValue() << ".";
    }
    std::string actualDescription = actualResponse.status().errorDescription();
    std::string expectedPartOfDescription = expectedResponse.status().errorDescription();

    if (actualDescription.find(expectedPartOfDescription) == std::string::npos)
    {
        return ::testing::AssertionFailure() << s.str() << " Expected response status error differ. "
                                             << expectedPartOfDescription << " Not inside the: " << actualDescription;
    }

    if (expectedResponse.data().columnData() != actualResponse.data().columnData())
    {
        return ::testing::AssertionFailure() << s.str() << " Expected response column data";
    }

    if (expectedResponse.data().columnHeaders() != actualResponse.data().columnHeaders())
    {
        return ::testing::AssertionFailure() << s.str() << " Expected response column data";
    }

    return ::testing::AssertionSuccess();
}

TEST_F(TestOSQueryProcessorWithMock, SimpleSelectShouldReturn) // NOLINT
{
    setupMockClient();
    auto& mockClient = m_mockClientPtr;

    std::string sql("select name,value from osquery_flags where name=='ephemeral'");
    osquery::QueryData queryData, queryColumnHeaders;
    queryColumnHeaders.push_back({ { "name", "TEXT" } });
    queryColumnHeaders.push_back({ { "value", "TEXT" } });
    osquery::Row row = { { "name", "ephemeral" }, { "value", "true" } };
    queryData.emplace_back(row);

    EXPECT_CALL(*mockClient, connect(m_testFakeSocketPath));
    EXPECT_CALL(*mockClient, query(sql, _))
        .WillOnce(DoAll(::testing::SetArgReferee<1>(queryData), Return(osquery::Status::success())));
    EXPECT_CALL(*mockClient, getQueryColumns(sql, _))
        .WillOnce(DoAll(::testing::SetArgReferee<1>(queryColumnHeaders), Return(osquery::Status::success())));

    osqueryclient::OsqueryProcessor osqueryProcessor(m_testFakeSocketPath);
    auto response = osqueryProcessor.query("select name,value from osquery_flags where name=='ephemeral'");

    livequery::ResponseData::ColumnData columnData;
    columnData.push_back({ { "name", "ephemeral" }, { "value", "true" } });
    auto expectedResponse = success(
        { { "name", livequery::ResponseData::AcceptedTypes::STRING },
          { "value", livequery::ResponseData::AcceptedTypes::STRING } },
        columnData);
    EXPECT_PRED_FORMAT2(responseIsEquivalent, response, expectedResponse);
}

TEST_F(TestOSQueryProcessorWithMock, NoOSqueryAvailableShouldReturnError) // NOLINT
{
    setupMockClient();
    auto& mockClient = m_mockClientPtr;

    EXPECT_CALL(*mockClient, connect(_))
        .WillOnce(Throw(
            osqueryclient::FailedToStablishConnectionException("\"Could not connect to osquery after 1 second\"")));

    std::string m_testFakeSocketPath("/tmp/osquery.sock");
    osqueryclient::OsqueryProcessor osqueryProcessor(m_testFakeSocketPath);
    auto response = osqueryProcessor.query("select name,value from osquery_flags where name=='ephemeral'");

    auto expectedResponse = failure(livequery::ErrorCode::UNEXPECTEDERROR, "connect");
    EXPECT_PRED_FORMAT2(responseIsEquivalent, response, expectedResponse);
}
TEST_F(TestOSQueryProcessorWithMock, UnsuccessfulExecutionsStatusFromQueryWillReturnErrorResponse) // NOLINT
{
    setupMockClient();
    auto& mockClient = m_mockClientPtr;

    EXPECT_CALL(*mockClient, connect(m_testFakeSocketPath));
    EXPECT_CALL(*mockClient, query(_, _)).WillOnce(Return(osquery::Status::failure("syntax error")));

    osqueryclient::OsqueryProcessor osqueryProcessor(m_testFakeSocketPath);
    auto response = osqueryProcessor.query("foo");
    auto expectedResponse = failure(livequery::ErrorCode::OSQUERYERROR, "syntax error");
    EXPECT_PRED_FORMAT2(responseIsEquivalent, response, expectedResponse);
}

TEST_F(TestOSQueryProcessorWithMock, UnsuccessfulExecutionsStatusFromGetQueryColumnsWillReturnError) // NOLINT
{
    setupMockClient();
    auto& mockClient = m_mockClientPtr;

    osquery::QueryData queryData;
    EXPECT_CALL(*mockClient, connect(m_testFakeSocketPath));
    EXPECT_CALL(*mockClient, query(_, _))
        .WillOnce(DoAll(::testing::SetArgReferee<1>(queryData), Return(osquery::Status::success())));
    EXPECT_CALL(*mockClient, getQueryColumns(_, queryData))
        .WillOnce(Return(osquery::Status::failure("error retrieving query column details")));

    osqueryclient::OsqueryProcessor osqueryProcessor(m_testFakeSocketPath);
    auto response = osqueryProcessor.query("select * from custom_sql");
    auto expectedResponse = failure(livequery::ErrorCode::OSQUERYERROR, "error retrieving query column details");
    EXPECT_PRED_FORMAT2(responseIsEquivalent, response, expectedResponse);
}

TEST_F(TestOSQueryProcessorWithMock, OsQueryClientExceptionsFromQueryingDataAreHandled) // NOLINT
{
    setupMockClient();
    auto& mockClient = m_mockClientPtr;
    std::string insaneQuery { R"(WITH RECURSIVE
counting (curr, next)
AS
( SELECT 1,1
  UNION ALL
  SELECT next, curr+1 FROM counting
  LIMIT 10000000000 )
SELECT group_concat(curr) FROM counting;)" };

    EXPECT_CALL(*mockClient, connect(m_testFakeSocketPath));
    EXPECT_CALL(*mockClient, query(_, _)).WillOnce(Throw(apache::thrift::transport::TTransportException()));

    osqueryclient::OsqueryProcessor osqueryProcessor(m_testFakeSocketPath);
    auto response = osqueryProcessor.query(insaneQuery);
    auto expectedResponse = failure(livequery::ErrorCode::EXTENSIONEXITEDWHILERUNNING, "running query");
    EXPECT_PRED_FORMAT2(responseIsEquivalent, response, expectedResponse);
}

TEST_F(TestOSQueryProcessorWithMock, OsQueryClientExceptionsFromGetQueryColumnsAreHandled) // NOLINT
{
    setupMockClient();
    auto& mockClient = m_mockClientPtr;

    osquery::QueryData queryData;
    std::string sql("select name,value from osquery_flags where name=='ephemeral'");
    EXPECT_CALL(*mockClient, connect(m_testFakeSocketPath));
    EXPECT_CALL(*mockClient, query(sql, _))
        .WillOnce(DoAll(::testing::SetArgReferee<1>(queryData), Return(osquery::Status::success())));
    EXPECT_CALL(*mockClient, getQueryColumns(sql, queryData))
        .WillOnce(Throw(apache::thrift::transport::TTransportException()));

    osqueryclient::OsqueryProcessor osqueryProcessor(m_testFakeSocketPath);
    auto response = osqueryProcessor.query(sql);
    auto expectedResponse = failure(livequery::ErrorCode::EXTENSIONEXITEDWHILERUNNING, "");
    EXPECT_PRED_FORMAT2(responseIsEquivalent, response, expectedResponse);
}

TEST_F(TestOSQueryProcessorWithMock, VerifyOSQueryResponseHasExpectedTypesForINTEGERandBIGINT) // NOLINT
{
    setupMockClient();
    auto& mockClient = m_mockClientPtr;

    std::string sql("select core,user from cpu_time Limit 1");

    osquery::QueryData queryData, queryColumnHeaders;
    queryColumnHeaders.push_back({ { "core", "INTEGER" } });
    queryColumnHeaders.push_back({ { "user", "BIGINT" } });

    osquery::Row row2 = { { "core", "00" } };
    osquery::Row row1 = { { "user", "133656" } };

    queryData.emplace_back(row1);
    queryData.emplace_back(row2);

    EXPECT_CALL(*mockClient, connect(m_testFakeSocketPath));
    EXPECT_CALL(*mockClient, query(sql, _))
        .WillOnce(DoAll(::testing::SetArgReferee<1>(queryData), Return(osquery::Status::success())));
    EXPECT_CALL(*mockClient, getQueryColumns(sql, _))
        .WillOnce(DoAll(::testing::SetArgReferee<1>(queryColumnHeaders), Return(osquery::Status::success())));

    osqueryclient::OsqueryProcessor osqueryProcessor(m_testFakeSocketPath);
    auto response = osqueryProcessor.query(sql);

    livequery::ResponseData::ColumnHeaders expectedHeaders;
    expectedHeaders.push_back({ "core", livequery::ResponseData::AcceptedTypes::INTEGER });
    expectedHeaders.push_back({ "user", livequery::ResponseData::AcceptedTypes::BIGINT });
    EXPECT_EQ(expectedHeaders, response.data().columnHeaders());
}

TEST_F(
    TestOSQueryProcessorWithMock,
    HeaderWillBeDeducedIfNotCompletellyGivenByOsQueryAndTheTypeWillAlwaysBeText) // NOLINT
{
    setupMockClient();
    auto& mockClient = m_mockClientPtr;

    std::string sql("select user from cpu_time Limit 1;select core from cpu_time Limit 1");
    osquery::QueryData queryData, queryColumnHeaders;
    queryColumnHeaders.push_back({ { "user", "BIGINT" } });
    osquery::Row row1 = { { "user", "133656" } };
    osquery::Row row2 = { { "core", "00" } };
    queryData.emplace_back(row1);
    queryData.emplace_back(row2);

    EXPECT_CALL(*mockClient, connect(m_testFakeSocketPath));
    EXPECT_CALL(*mockClient, query(sql, _))
        .WillOnce(DoAll(::testing::SetArgReferee<1>(queryData), Return(osquery::Status::success())));
    EXPECT_CALL(*mockClient, getQueryColumns(sql, _))
        .WillOnce(DoAll(::testing::SetArgReferee<1>(queryColumnHeaders), Return(osquery::Status::success())));

    // osquery.getQueryColumns will only return information for user and type BIGINT
    // the information from core will be 'deduced'.
    osqueryclient::OsqueryProcessor osqueryProcessor(m_testFakeSocketPath);
    auto response = osqueryProcessor.query(sql);
    auto& responseColumnData = response.data().columnData();

    livequery::ResponseData::ColumnData columnData;
    // I want to verify only the structure and the headers, not the value.
    columnData.push_back({ { "user", responseColumnData.at(0).at("user") } });
    columnData.push_back({ { "core", responseColumnData.at(1).at("core") } });
    livequery::ResponseData::ColumnHeaders headers {
        { "user", livequery::ResponseData::AcceptedTypes::BIGINT },
        { "core", livequery::ResponseData::AcceptedTypes::STRING }, // core was deduced from the columnData
    };
    auto expectedResponse = success(headers, columnData);
    EXPECT_PRED_FORMAT2(responseIsEquivalent, response, expectedResponse);
}