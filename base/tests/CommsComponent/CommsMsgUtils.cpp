/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "CommsMsgUtils.h"

using namespace CommsComponent;

::testing::AssertionResult requestAreEquivalent(
        std::stringstream & s,
        const Common::HttpSender::RequestConfig& expected,
        const Common::HttpSender::RequestConfig& actual)
{
    if( expected.getAdditionalHeaders() != actual.getAdditionalHeaders())
    {
        s << "headers differ. Expected: " << expected.getAdditionalHeaders().size() <<  " Actual: " << actual.getAdditionalHeaders().size();
        return ::testing::AssertionFailure() << s.str();
    }
    if( expected.getCertPath() != actual.getCertPath())
    {
        s << "getCertPath differ. Expected: " << expected.getCertPath() <<  " Actual: " << actual.getCertPath();
        return ::testing::AssertionFailure() << s.str();
    }
    if( expected.getData() != actual.getData())
    {
        s << "getData differ. Expected: " << expected.getData() <<  " Actual: " << actual.getData();
        return ::testing::AssertionFailure() << s.str();
    }
    if( expected.getPort() != actual.getPort())
    {
        s << "getPort differ. Expected: " << expected.getPort() <<  " Actual: " << actual.getPort();
        return ::testing::AssertionFailure() << s.str();
    }
    if( expected.getRequestType() != actual.getRequestType())
    {
        s << "getRequestType differ. Expected: " << expected.getRequestTypeAsString() <<  " Actual: " << actual.getRequestTypeAsString();
        return ::testing::AssertionFailure() << s.str();
    }
    if( expected.getResourcePath() != actual.getResourcePath())
    {
        s << "getResourcePath differ. Expected: " << expected.getResourcePath() <<  " Actual: " << actual.getResourcePath();
        return ::testing::AssertionFailure() << s.str();
    }
    if( expected.getServer() != actual.getServer())
    {
        s << "getServer differ. Expected: " << expected.getServer() <<  " Actual: " << actual.getServer();
        return ::testing::AssertionFailure() << s.str();
    }

    return ::testing::AssertionSuccess();
}

::testing::AssertionResult responseAreEquivalent(
        std::stringstream & s,
        const Common::HttpSender::HttpResponse& expected,
        const Common::HttpSender::HttpResponse& actual)
{
    if (expected.httpCode != actual.httpCode)
    {
        s << "httpCode differs. expected: " << expected.httpCode << " actual " << actual.httpCode;
        return ::testing::AssertionFailure() << s.str();
    }
    if (expected.exitCode != actual.exitCode)
    {
        s << "exitCode differs. expected: " << expected.exitCode << " actual " << actual.exitCode;
        return ::testing::AssertionFailure() << s.str();
    }
    if (expected.description != actual.description)
    {
        s << "description differs. expected: " << expected.description << " actual " << actual.description;
        return ::testing::AssertionFailure() << s.str();
    }
    if (expected.bodyContent != actual.bodyContent)
    {
        s << "bodyContent differs. expected: " << expected.bodyContent << " actual " << actual.bodyContent;
        return ::testing::AssertionFailure() << s.str();
    }


    return ::testing::AssertionSuccess();
}

::testing::AssertionResult configAreEquivalent(
        std::stringstream & s,
        const CommsComponent::CommsConfig& expected,
        const CommsComponent::CommsConfig& actual)
{
    if (expected.getKeyList() != actual.getKeyList())
    {
        s << "KeyList differs. expected: " << expected.getKeyList().size() << " actual " << actual.getKeyList().size();
        return ::testing::AssertionFailure() << s.str();
    }

    return ::testing::AssertionSuccess();
}
::testing::AssertionResult commsMsgAreEquivalent(
        const char* m_expr,
        const char* n_expr,
        const CommsComponent::CommsMsg& expected,
        const CommsComponent::CommsMsg& actual) {
    std::stringstream s;
    s << m_expr << " and " << n_expr << " failed: ";

    if (actual.id != expected.id)
    {
        return ::testing::AssertionFailure() << s.str() << "the ids are not the same";
    }
    if (expected.content.index() != actual.content.index())
    {
        s << "Not holding the same content variant";
        return ::testing::AssertionFailure() << s.str();
    }
    if (std::holds_alternative<Common::HttpSender::RequestConfig>(expected.content))
    {
        return requestAreEquivalent(s, std::get<Common::HttpSender::RequestConfig>(expected.content), std::get<Common::HttpSender::RequestConfig>(actual.content));
    }
    else if (std::holds_alternative<Common::HttpSender::HttpResponse>(expected.content))
    {
        return responseAreEquivalent(s, std::get<Common::HttpSender::HttpResponse>(expected.content), std::get<Common::HttpSender::HttpResponse>(actual.content));
    }
    else
    {
        return configAreEquivalent(s, std::get<CommsComponent::CommsConfig>(expected.content), std::get<CommsComponent::CommsConfig>(actual.content));
    }

}