/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <gtest/gtest.h>
#include <tests/Common/Helpers/LogInitializedTests.h>
#include <tests/Common/Helpers/MockFileSystem.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include "CommsMsgUtils.h"

using namespace CommsComponent;

class TestCommsMsg : public LogOffInitializedTests
{
};

CommsMsg serializeAndDeserialize(const CommsMsg& input)
{
    auto serialized = CommsComponent::CommsMsg::serialize(input);
    return CommsComponent::CommsMsg::fromString(serialized);
}

TEST_F(TestCommsMsg, DefaultCommsCanBeProcessedCorrectly) // NOLINT
{
    CommsMsg input;
    CommsMsg result = serializeAndDeserialize(input);
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
}

TEST_F(TestCommsMsg, IDofCommsCanBeProcessedCorrectly) // NOLINT
{
    CommsMsg input;
    input.id = "stuff";
    CommsMsg result = serializeAndDeserialize(input);
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
    EXPECT_EQ(result.id,"stuff");
}

TEST_F(TestCommsMsg, bodyContentOfHttpResponseCanBeProcessedCorrectly) // NOLINT
{
    CommsMsg input;
    Common::HttpSender::HttpResponse httpResponse;
    httpResponse.bodyContent = "stuff";
    input.content = httpResponse;
    CommsMsg result = serializeAndDeserialize(input);
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
    EXPECT_EQ(std::get<Common::HttpSender::HttpResponse>(result.content).bodyContent,"stuff");
    EXPECT_EQ(std::get<Common::HttpSender::HttpResponse>(result.content).bodyContent,httpResponse.bodyContent);
}

TEST_F(TestCommsMsg, httpCodeOfHttpResponseCanBeProcessedCorrectly) // NOLINT
{
    CommsMsg input;
    Common::HttpSender::HttpResponse httpResponse;
    httpResponse.httpCode = 400;
    input.content = httpResponse;
    CommsMsg result = serializeAndDeserialize(input);
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
    EXPECT_EQ(std::get<Common::HttpSender::HttpResponse>(result.content).httpCode,400);
}

TEST_F(TestCommsMsg, exitCodefHttpResponseCanBeProcessedCorrectly) // NOLINT
{
    CommsMsg input;
    Common::HttpSender::HttpResponse httpResponse;
    httpResponse.exitCode = 3;
    input.content = httpResponse;
    CommsMsg result = serializeAndDeserialize(input);
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
    EXPECT_EQ(std::get<Common::HttpSender::HttpResponse>(result.content).exitCode,3);
}

TEST_F(TestCommsMsg, descriptionOfHttpResponseCanBeProcessedCorrectly) // NOLINT
{
    CommsMsg input;
    Common::HttpSender::HttpResponse httpResponse;
    httpResponse.description = "stuff";
    input.content = httpResponse;
    CommsMsg result = serializeAndDeserialize(input);
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
    EXPECT_EQ(std::get<Common::HttpSender::HttpResponse>(result.content).description,"stuff");
}

TEST_F(TestCommsMsg, bodyContentOfHttpRequestCanBeProcessedCorrectly) // NOLINT
{
    CommsMsg input;
    Common::HttpSender::RequestConfig requestConfig;
    requestConfig.setData("stuff");
    input.content = requestConfig;
    CommsMsg result = serializeAndDeserialize(input);
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
    EXPECT_EQ(std::get<Common::HttpSender::RequestConfig>(result.content).getData(),"stuff");
}
TEST_F(TestCommsMsg, requestTypeOfHttpRequestCanBeProcessedCorrectly) // NOLINT
{
    CommsMsg input;
    Common::HttpSender::RequestConfig requestConfig;
    requestConfig.setRequestTypeFromString("GET");
    input.content = requestConfig;
    CommsMsg result = serializeAndDeserialize(input);
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
    EXPECT_EQ(std::get<Common::HttpSender::RequestConfig>(result.content).getRequestTypeAsString(),"GET");
}

TEST_F(TestCommsMsg, HeadersOfHttpRequestCanBeProcessedCorrectly) // NOLINT
{
    CommsMsg input;
    Common::HttpSender::RequestConfig requestConfig;
    std::vector<std::string> headers;
    headers.push_back("header");
    headers.push_back("header1");
    requestConfig.setAdditionalHeaders(headers);
    input.content = requestConfig;
    CommsMsg result = serializeAndDeserialize(input);
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
    EXPECT_EQ(std::get<Common::HttpSender::RequestConfig>(result.content).getAdditionalHeaders(),headers);
}

TEST_F(TestCommsMsg, serverOfHttpRequestCanBeProcessedCorrectly) // NOLINT
{
    CommsMsg input;
    Common::HttpSender::RequestConfig requestConfig;
    requestConfig.setServer("testserver.com");
    input.content = requestConfig;
    CommsMsg result = serializeAndDeserialize(input);
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
    EXPECT_EQ(std::get<Common::HttpSender::RequestConfig>(result.content).getServer(),"testserver.com");
}

TEST_F(TestCommsMsg, resourcePathOfHttpRequestCanBeProcessedCorrectly) // NOLINT
{
    CommsMsg input;
    Common::HttpSender::RequestConfig requestConfig;
    requestConfig.setResourcePath("/blah/blah1");
    input.content = requestConfig;
    CommsMsg result = serializeAndDeserialize(input);
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
    EXPECT_EQ(std::get<Common::HttpSender::RequestConfig>(result.content).getResourcePath(),"/blah/blah1");
}

TEST_F(TestCommsMsg, certPathOfHttpRequestCanBeProcessedCorrectly) // NOLINT
{
    CommsMsg input;
    Common::HttpSender::RequestConfig requestConfig;
    requestConfig.setCertPath("/tmp/cert.pem");
    input.content = requestConfig;
    CommsMsg result = serializeAndDeserialize(input);
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
    EXPECT_EQ(std::get<Common::HttpSender::RequestConfig>(result.content).getCertPath(),"/tmp/cert.pem");
}

TEST_F(TestCommsMsg, portOfHttpRequestCanBeProcessedCorrectly) // NOLINT
{
    CommsMsg input;
    Common::HttpSender::RequestConfig requestConfig;
    requestConfig.setPort(433);
    input.content = requestConfig;
    CommsMsg result = serializeAndDeserialize(input);
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
    EXPECT_EQ(std::get<Common::HttpSender::RequestConfig>(result.content).getPort(),433);
}


TEST_F(TestCommsMsg, serializationAndDeserializationOfJsonShouldWorkForHttpResponse) // NOLINT
{
    Common::HttpSender::HttpResponse httpResponse; 
    httpResponse.description = "desc"; 
    httpResponse.httpCode = 3; 
    httpResponse.bodyContent = "body"; 
    CommsMsg input; 
    input.content = httpResponse; 
    Common::HttpSender::HttpResponse resHttp = CommsMsg::httpResponseFromJson(CommsMsg::toJson(httpResponse)); 
    CommsMsg result; 
    result.content = resHttp; 
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
}

TEST_F(TestCommsMsg, deserializeFromJsonForHttpResponse) // NOLINT
{
    Common::HttpSender::HttpResponse httpResponse; 
    httpResponse.description = "desc"; 
    httpResponse.httpCode = 3; 
    httpResponse.bodyContent = "body"; 
    // observe that body is the base64 encoded of the string body. This is to 
    // ensure that the body may contain binary strings. 
    std::string json = R"({
 "httpCode": "3",
 "description": "desc",
 "bodyContent": "Ym9keQ=="
})"; 
    Common::HttpSender::HttpResponse resHttp = CommsMsg::httpResponseFromJson(json); 
    CommsMsg input;     
    CommsMsg result; 
    input.content = httpResponse; 
    result.content = resHttp; 
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
}

TEST_F(TestCommsMsg, serializationAndDeserializationOfJsonShouldWorkForHttpRequest) // NOLINT
{
    Common::HttpSender::RequestConfig configRequest{
        Common::HttpSender::RequestType::POST, 
        std::vector<std::string>{"header1","header2"}, 
        "domain.com",
        335,
        "certPath", "/index.html"}; 
    configRequest.setData("data"); 
    CommsMsg input; 
    input.content = configRequest; 
    Common::HttpSender::RequestConfig reqHttp = CommsMsg::requestConfigFromJson(CommsMsg::toJson(configRequest)); 

    CommsMsg result; 
    result.content = reqHttp; 
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
}

TEST_F(TestCommsMsg, deserializeFromJsonForConfigRequest) // NOLINT
{
    Common::HttpSender::RequestConfig configRequest{
        Common::HttpSender::RequestType::POST, 
        std::vector<std::string>{"header1","header2"}, 
        "domain.com",
        335,
        "certPath1", "/index.html"}; 


    configRequest.setData("data"); 
    // observe that bodyContent is the base64 encoded of the string 'data'. 

    std::string json = R"({
 "requestType": "POST",
 "server": "domain.com",
 "resourcePath": "/index.html",
 "bodyContent": "ZGF0YQ==",
 "port": "335",
 "certPath" : "certPath1",
 "headers": [
  "header1",
  "header2"
 ]
})"; 
    Common::HttpSender::RequestConfig reqHttp = CommsMsg::requestConfigFromJson(json); 
    CommsMsg input;     
    CommsMsg result; 
    input.content = configRequest; 
    result.content = reqHttp; 
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
}
    
TEST_F(TestCommsMsg, keyValueOfConfigCanBeProcessedCorrectly) // NOLINT
{
    CommsMsg input;
    CommsConfig config;
    config.addKeyValueToList(std::pair<std::string,std::vector<std::string>>("stuff1",{"stuff"}));
    input.content = config;
    CommsMsg result = serializeAndDeserialize(input);
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
    std::map<std::string, std::vector<std::string>> key_composed_values;
    key_composed_values.insert(std::pair<std::string,std::vector<std::string>>("stuff1",{"stuff"}));
    EXPECT_EQ(std::get<CommsComponent::CommsConfig>(result.content).getKeyList(),key_composed_values);
}

TEST_F(TestCommsMsg, auxiliaryProxySettingsCanBeProcessedCorrectly) // NOLINT
{
    CommsMsg input;
    CommsConfig config;
    config.setProxy("ThisProxy"); 
    config.setDeobfuscatedCredential("user:pass"); 
    input.content = config;
    CommsMsg result = serializeAndDeserialize(input);
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
    EXPECT_EQ(config.getProxy(), "ThisProxy"); 
    EXPECT_EQ(config.getDeobfuscatedCredential(), "user:pass"); 
}


TEST_F(TestCommsMsg, keyValueOfProxyConfigCanBeProcessedCorrectly) // NOLINT
{
    CommsMsg input;
    CommsConfig config;
    auto filesystemMock = new StrictMock<MockFileSystem>();
    std::string content = R"({"proxy":"localhost","credentials":"CCD4E57ZjW+t5XPiMSJH1TurG3MfWCN3DpjJRINMwqNaWl+3zzlVIdyVmifCHUwcmaX6+YTSyyBM8SslIIGV5rUw"})";
    EXPECT_CALL(*filesystemMock, isFile(_)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(_)).WillOnce(Return(content));
    Tests::ScopedReplaceFileSystem ScopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};

    config.addProxyInfoToConfig();
    input.content = config;
    CommsMsg result = serializeAndDeserialize(input);
    EXPECT_PRED_FORMAT2(commsMsgAreEquivalent, input, result);
    std::map<std::string, std::vector<std::string>> key_composed_values;
    key_composed_values.insert(std::pair<std::string,std::vector<std::string>>("proxy",{"localhost"}));
    key_composed_values.insert(std::pair<std::string,std::vector<std::string>>("credentials",{"username:password"}));
    EXPECT_EQ(std::get<CommsComponent::CommsConfig>(result.content).getKeyList(),key_composed_values);
}
