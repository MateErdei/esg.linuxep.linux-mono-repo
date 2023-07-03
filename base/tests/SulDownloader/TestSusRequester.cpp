/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SulDownloader/sdds3/SusRequester.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/LogInitializedTests.h"

#include "Common/Logging/ConsoleLoggingSetup.h"
#include <gtest/gtest.h>
#include "tests/Common/Helpers/MockHttpRequester.h"


class SusRequesterTest : public LogInitializedTests{};


TEST_F(SusRequesterTest, susRequesterHandles200ResponseWithValidJson)
{
    auto mockHttpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    auto okResponse = Common::HttpRequests::Response();
    okResponse.status = Common::HttpRequests::HTTP_STATUS_OK;
    okResponse.body = R"({"suites": ["suite1", "suite2"], "release-groups": ["group1", "group2"]})";
    okResponse.errorCode = Common::HttpRequests::ResponseErrorCode::OK;
    EXPECT_CALL(*mockHttpRequester, post(_)).WillOnce(Return(okResponse));

    SulDownloader::SDDS3::SusRequester susRequester(mockHttpRequester);
    SulDownloader::SUSRequestParameters requestParameters;

    auto response = susRequester.request(requestParameters);
    ASSERT_TRUE(response.success);

    SulDownloader::SDDS3::SusData responseSusData;
    responseSusData.suites = {"suite1","suite2"};
    responseSusData.releaseGroups = {"group1","group2"};
    ASSERT_EQ(response.data.suites, responseSusData.suites);
    ASSERT_EQ(response.data.releaseGroups, responseSusData.releaseGroups);
}

TEST_F(SusRequesterTest, susRequesterHandles200ResponseWithInvalidJson)
{
    auto mockHttpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    auto okResponse = Common::HttpRequests::Response();
    okResponse.status = Common::HttpRequests::HTTP_STATUS_OK;
    okResponse.body = "not json";
    okResponse.errorCode = Common::HttpRequests::ResponseErrorCode::OK;
    EXPECT_CALL(*mockHttpRequester, post(_)).WillOnce(Return(okResponse));

    SulDownloader::SDDS3::SusRequester susRequester(mockHttpRequester);
    SulDownloader::SUSRequestParameters requestParameters;
    auto response = susRequester.request(requestParameters);
    ASSERT_FALSE(response.success);
    ASSERT_TRUE(response.data.releaseGroups.empty());
    ASSERT_TRUE(response.data.suites.empty());
}

TEST_F(SusRequesterTest, susRequesterHandlesFailedRequest)
{
    auto mockHttpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    auto failedResponse = Common::HttpRequests::Response();
    failedResponse.errorCode = Common::HttpRequests::ResponseErrorCode::REQUEST_FAILED;
    EXPECT_CALL(*mockHttpRequester, post(_)).WillOnce(Return(failedResponse));

    SulDownloader::SDDS3::SusRequester susRequester(mockHttpRequester);
    SulDownloader::SUSRequestParameters requestParameters;
    auto response = susRequester.request(requestParameters);
    ASSERT_FALSE(response.success);
    ASSERT_TRUE(response.data.releaseGroups.empty());
    ASSERT_TRUE(response.data.suites.empty());
}

TEST_F(SusRequesterTest, susRequesterHandlesNon200Response)
{
    auto mockHttpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    auto failedResponse = Common::HttpRequests::Response();
    failedResponse.status = 500;
    failedResponse.errorCode = Common::HttpRequests::ResponseErrorCode::OK;
    EXPECT_CALL(*mockHttpRequester, post(_)).WillOnce(Return(failedResponse));
    SulDownloader::SDDS3::SusRequester susRequester(mockHttpRequester);
    SulDownloader::SUSRequestParameters requestParameters;
    auto response = susRequester.request(requestParameters);
    ASSERT_FALSE(response.success);
    ASSERT_TRUE(response.data.releaseGroups.empty());
    ASSERT_TRUE(response.data.suites.empty());
}


