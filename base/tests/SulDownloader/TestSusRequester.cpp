// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "JwtUtils.h"
#include "MockSignatureVerifierWrapper.h"

#include "Common/Logging/ConsoleLoggingSetup.h"
#include "Common/SslImpl/Digest.h"
#include "SulDownloader/sdds3/SusRequester.h"
#include "sophlib/crypto/Base64.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/SystemUtilsReplaceAndRestore.h"
#include "tests/Common/Helpers/LogInitializedTests.h"
#include "tests/Common/Helpers/MockFileSystem.h"
#include "tests/Common/Helpers/MockHttpRequester.h"
#include "tests/Common/Helpers/MockSystemUtils.h"

#include <gtest/gtest.h>

namespace
{
    class SusRequesterTest : public LogInitializedTests
    {
    protected:
        void SetUp() override
        {
            verifier_ = std::make_unique<StrictMock<MockSignatureVerifierWrapper>>();
            mockFileSystem_ = std::make_unique<StrictMock<MockFileSystem>>();
        }

        void TearDown() override
        {
            Tests::restoreSystemUtils();
            Tests::restoreFileSystem();
        }
        std::unique_ptr<StrictMock<MockSignatureVerifierWrapper>> verifier_;
        std::unique_ptr<StrictMock<MockFileSystem>> mockFileSystem_;
    };
} // namespace

TEST_F(SusRequesterTest, susRequesterHandles200ResponseWithValidJson)
{
    auto mockHttpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    auto okResponse = Common::HttpRequests::Response();
    okResponse.status = Common::HttpRequests::HTTP_STATUS_OK;
    okResponse.body = R"({"suites": ["suite1", "suite2"], "release-groups": ["group1", "group2"]})";
    okResponse.headers["X-Content-Signature"] = generateJwt(okResponse.body);
    okResponse.errorCode = Common::HttpRequests::ResponseErrorCode::OK;
    EXPECT_CALL(*mockHttpRequester, post(_)).WillOnce(Return(okResponse));
    EXPECT_CALL(*verifier_, Verify(_, _));

    SulDownloader::SDDS3::SusRequester susRequester(mockHttpRequester, std::move(verifier_));
    SulDownloader::SUSRequestParameters requestParameters;

    auto response = susRequester.request(requestParameters);
    ASSERT_TRUE(response.success);

    SulDownloader::SDDS3::SusData responseSusData;
    responseSusData.suites = { "suite1", "suite2" };
    responseSusData.releaseGroups = { "group1", "group2" };
    ASSERT_EQ(response.data.suites, responseSusData.suites);
    ASSERT_EQ(response.data.releaseGroups, responseSusData.releaseGroups);
}

TEST_F(SusRequesterTest, susRequesterHandles200ResponseWithInvalidJson)
{
    auto mockHttpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    auto okResponse = Common::HttpRequests::Response();
    okResponse.status = Common::HttpRequests::HTTP_STATUS_OK;
    okResponse.body = "not json";
    okResponse.headers["X-Content-Signature"] = generateJwt(okResponse.body);
    okResponse.errorCode = Common::HttpRequests::ResponseErrorCode::OK;
    EXPECT_CALL(*mockHttpRequester, post(_)).WillOnce(Return(okResponse));
    EXPECT_CALL(*verifier_, Verify(_, _));

    SulDownloader::SDDS3::SusRequester susRequester(mockHttpRequester, std::move(verifier_));
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
    failedResponse.headers["X-Content-Signature"] = generateJwt(failedResponse.body);
    failedResponse.errorCode = Common::HttpRequests::ResponseErrorCode::REQUEST_FAILED;
    EXPECT_CALL(*mockHttpRequester, post(_)).WillOnce(Return(failedResponse));
    EXPECT_CALL(*verifier_, Verify(_, _)).Times(0);

    SulDownloader::SDDS3::SusRequester susRequester(mockHttpRequester, std::move(verifier_));
    SulDownloader::SUSRequestParameters requestParameters;
    auto response = susRequester.request(requestParameters);
    ASSERT_FALSE(response.success);
    ASSERT_TRUE(response.data.releaseGroups.empty());
    ASSERT_TRUE(response.data.suites.empty());
}

TEST_F(SusRequesterTest, susRequesterHandlesCannotReachServer)
{
    auto mockHttpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    auto failedResponse = Common::HttpRequests::Response();
    failedResponse.headers["X-Content-Signature"] = generateJwt(failedResponse.body);
    failedResponse.errorCode = Common::HttpRequests::ResponseErrorCode::COULD_NOT_RESOLVE_HOST;
    EXPECT_CALL(*mockHttpRequester, post(_)).WillOnce(Return(failedResponse));
    EXPECT_CALL(*verifier_, Verify(_, _)).Times(0);

    SulDownloader::SDDS3::SusRequester susRequester(mockHttpRequester, std::move(verifier_));
    SulDownloader::SUSRequestParameters requestParameters;
    auto response = susRequester.request(requestParameters);
    ASSERT_FALSE(response.success);
    ASSERT_FALSE(response.persistentError);
    ASSERT_TRUE(response.data.releaseGroups.empty());
    ASSERT_TRUE(response.data.suites.empty());
}

TEST_F(SusRequesterTest, susRequesterHandlesCannotReachProxy)
{
    auto mockHttpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    auto failedResponse = Common::HttpRequests::Response();
    failedResponse.headers["X-Content-Signature"] = generateJwt(failedResponse.body);
    failedResponse.errorCode = Common::HttpRequests::ResponseErrorCode::COULD_NOT_RESOLVE_PROXY;
    EXPECT_CALL(*mockHttpRequester, post(_)).WillOnce(Return(failedResponse));
    EXPECT_CALL(*verifier_, Verify(_, _)).Times(0);

    SulDownloader::SDDS3::SusRequester susRequester(mockHttpRequester, std::move(verifier_));
    SulDownloader::SUSRequestParameters requestParameters;
    auto response = susRequester.request(requestParameters);
    ASSERT_FALSE(response.success);
    ASSERT_FALSE(response.persistentError);
    ASSERT_TRUE(response.data.releaseGroups.empty());
    ASSERT_TRUE(response.data.suites.empty());
}

TEST_F(SusRequesterTest, susRequesterHandlesTimeout)
{
    auto mockHttpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    auto failedResponse = Common::HttpRequests::Response();
    failedResponse.headers["X-Content-Signature"] = generateJwt(failedResponse.body);
    failedResponse.errorCode = Common::HttpRequests::ResponseErrorCode::TIMEOUT;
    EXPECT_CALL(*mockHttpRequester, post(_)).WillOnce(Return(failedResponse));
    EXPECT_CALL(*verifier_, Verify(_, _)).Times(0);

    SulDownloader::SDDS3::SusRequester susRequester(mockHttpRequester, std::move(verifier_));
    SulDownloader::SUSRequestParameters requestParameters;
    auto response = susRequester.request(requestParameters);
    ASSERT_FALSE(response.success);
    ASSERT_FALSE(response.persistentError);
    ASSERT_TRUE(response.data.releaseGroups.empty());
    ASSERT_TRUE(response.data.suites.empty());
}

TEST_F(SusRequesterTest, susRequesterHandlesNon200Response)
{
    auto mockHttpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    auto failedResponse = Common::HttpRequests::Response();
    failedResponse.status = 400;
    failedResponse.headers["X-Content-Signature"] = generateJwt(failedResponse.body);
    failedResponse.errorCode = Common::HttpRequests::ResponseErrorCode::OK;
    EXPECT_CALL(*mockHttpRequester, post(_)).WillOnce(Return(failedResponse));
    EXPECT_CALL(*verifier_, Verify(_, _)).Times(0);
    SulDownloader::SDDS3::SusRequester susRequester(mockHttpRequester, std::move(verifier_));
    SulDownloader::SUSRequestParameters requestParameters;
    auto response = susRequester.request(requestParameters);
    ASSERT_FALSE(response.success);
    ASSERT_TRUE(response.persistentError);
    ASSERT_TRUE(response.data.releaseGroups.empty());
    ASSERT_TRUE(response.data.suites.empty());
}

TEST_F(SusRequesterTest, susRequesterHandlesServerError)
{
    auto mockHttpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    auto failedResponse = Common::HttpRequests::Response();
    failedResponse.status = 500;
    failedResponse.headers["X-Content-Signature"] = generateJwt(failedResponse.body);
    failedResponse.errorCode = Common::HttpRequests::ResponseErrorCode::OK;
    EXPECT_CALL(*mockHttpRequester, post(_)).WillOnce(Return(failedResponse));
    EXPECT_CALL(*verifier_, Verify(_, _)).Times(0);
    SulDownloader::SDDS3::SusRequester susRequester(mockHttpRequester, std::move(verifier_));
    SulDownloader::SUSRequestParameters requestParameters;
    auto response = susRequester.request(requestParameters);
    ASSERT_FALSE(response.success);
    ASSERT_FALSE(response.persistentError);
    ASSERT_TRUE(response.data.releaseGroups.empty());
    ASSERT_TRUE(response.data.suites.empty());
}

// This is required because real SUS sends the header as lowercase 'x-content-signature'
TEST_F(SusRequesterTest, LowerCaseXContentSignatureIsHandledSuccessfully)
{
    auto mockHttpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    auto okResponse = Common::HttpRequests::Response();
    okResponse.status = Common::HttpRequests::HTTP_STATUS_OK;
    okResponse.body = R"({"suites": ["suite1", "suite2"], "release-groups": ["group1", "group2"]})";
    okResponse.headers["x-content-signature"] = generateJwt(okResponse.body);
    okResponse.errorCode = Common::HttpRequests::ResponseErrorCode::OK;
    EXPECT_CALL(*mockHttpRequester, post(_)).WillOnce(Return(okResponse));
    EXPECT_CALL(*verifier_, Verify(_, _));

    SulDownloader::SDDS3::SusRequester susRequester(mockHttpRequester, std::move(verifier_));
    SulDownloader::SUSRequestParameters requestParameters;

    auto response = susRequester.request(requestParameters);
    ASSERT_TRUE(response.success);

    SulDownloader::SDDS3::SusData responseSusData;
    responseSusData.suites = { "suite1", "suite2" };
    responseSusData.releaseGroups = { "group1", "group2" };
    ASSERT_EQ(response.data.suites, responseSusData.suites);
    ASSERT_EQ(response.data.releaseGroups, responseSusData.releaseGroups);
}

// Based on
// esg.em.esg/products/windows_endpoint/protection/sau/SDDSDownloaderTests/SDDS3DownloaderTests.cpp:InvalidServiceSignatures
TEST_F(SusRequesterTest, InvalidServiceSignatures)
{
    auto okResponse = Common::HttpRequests::Response();
    okResponse.status = Common::HttpRequests::HTTP_STATUS_OK;
    okResponse.body = R"({"suites": ["suite1", "suite2"], "release-groups": ["group1", "group2"]})";
    okResponse.errorCode = Common::HttpRequests::ResponseErrorCode::OK;

    SulDownloader::SUSRequestParameters requestParameters;

    // No X-Content-Signature header
    {
        auto mockHttpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
        EXPECT_CALL(*mockHttpRequester, post(_)).WillOnce(Return(okResponse));

        auto verifier = std::make_unique<StrictMock<MockSignatureVerifierWrapper>>();

        SulDownloader::SDDS3::SusRequester susRequester(mockHttpRequester, std::move(verifier));
        auto response = susRequester.request(requestParameters);
        EXPECT_FALSE(response.success);
        EXPECT_EQ(response.error, "Failed to verify SUS response: SUS response does not have X-Content-Signature");
    }

    // Doesn't contain three '.'-separated fields
    for (auto& sig : { "foo", "foo.bar", "foo.bar.baz.com" })
    {
        okResponse.headers["X-Content-Signature"] = sig;
        auto mockHttpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
        EXPECT_CALL(*mockHttpRequester, post(_)).WillOnce(Return(okResponse));

        auto verifier = std::make_unique<StrictMock<MockSignatureVerifierWrapper>>();

        SulDownloader::SDDS3::SusRequester susRequester(mockHttpRequester, std::move(verifier));
        auto response = susRequester.request(requestParameters);
        EXPECT_FALSE(response.success);
        EXPECT_THAT(
            response.error,
            StartsWith("Failed to verify JWT in SUS response: JWT does not consist of three parts, parts found: "));
    }

    // Header contains invalid content (not base64url JSON)
    {
        okResponse.headers["X-Content-Signature"] = "f.o.o"; // "f" can't be decoded as json-in-base64url
        auto mockHttpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
        EXPECT_CALL(*mockHttpRequester, post(_)).WillOnce(Return(okResponse));

        auto verifier = std::make_unique<StrictMock<MockSignatureVerifierWrapper>>();

        SulDownloader::SDDS3::SusRequester susRequester(mockHttpRequester, std::move(verifier));
        auto response = susRequester.request(requestParameters);
        EXPECT_FALSE(response.success);
        EXPECT_EQ(response.error, "Failed to verify JWT in SUS response: Invalid base64 string length 1");
    }

    // Header contains invalid JSON
    {
        okResponse.headers["X-Content-Signature"] =
            "foo.bar.baz"; // "foo" is valid base64url but doesn't decode to valid JSON
        auto mockHttpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
        EXPECT_CALL(*mockHttpRequester, post(_)).WillOnce(Return(okResponse));

        auto verifier = std::make_unique<StrictMock<MockSignatureVerifierWrapper>>();

        SulDownloader::SDDS3::SusRequester susRequester(mockHttpRequester, std::move(verifier));
        auto response = susRequester.request(requestParameters);
        EXPECT_FALSE(response.success);
        EXPECT_THAT(
            response.error, StartsWith("Failed to verify JWT in SUS response: [json.exception.parse_error.101]"));
    }

    // Header contains semantically incorrect JSON
    for (auto& header : {
             "[]",
             "null",
             "1",
             "\"string\"",
             "{}",
             R"j({"typ":null})j",
             R"j({"typ":1})j",
             R"j({"typ":"string"})j",
             R"j({"typ":"JWT"})j",
             R"j({"typ":"JWT","alg":null})j",
             R"j({"typ":"JWT","alg":1})j",
             R"j({"typ":"JWT","alg":"string"})j",
             R"j({"typ":"JWT","alg":"RS256"})j",
             R"j({"typ":"JWT","alg":"RS384"})j",
             R"j({"typ":"JWT","alg":"RS384","x5c":null})j",
             R"j({"typ":"JWT","alg":"RS384","x5c":1})j",
             R"j({"typ":"JWT","alg":"RS384","x5c":"string"})j",
             R"j({"typ":"JWT","alg":"RS384","x5c":{}})j",
         })
    {
        auto sig = sophlib::crypto::base64url::Encode(header) + ".foo.bar";
        okResponse.headers["X-Content-Signature"] = sig;
        auto mockHttpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
        EXPECT_CALL(*mockHttpRequester, post(_)).WillOnce(Return(okResponse));

        auto verifier = std::make_unique<StrictMock<MockSignatureVerifierWrapper>>();

        SulDownloader::SDDS3::SusRequester susRequester(mockHttpRequester, std::move(verifier));
        auto response = susRequester.request(requestParameters);
        EXPECT_FALSE(response.success);
        EXPECT_THAT(response.error, StartsWith("Failed to verify JWT in SUS response: Invalid JWT header: bad '"));
    }

    // Header's signature doesn't match the payload
    {
        okResponse.body = "dummy";
        okResponse.headers["X-Content-Signature"] = generateJwt("tummy");
        auto mockHttpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
        EXPECT_CALL(*mockHttpRequester, post(_)).WillOnce(Return(okResponse));

        auto verifier = std::make_unique<StrictMock<MockSignatureVerifierWrapper>>();
        EXPECT_CALL(*verifier, Verify(_, _));

        SulDownloader::SDDS3::SusRequester susRequester(mockHttpRequester, std::move(verifier));
        auto response = susRequester.request(requestParameters);
        EXPECT_FALSE(response.success);
        EXPECT_EQ(
            response.error,
            "Failed to verify SUS response: Malformed SUS response: sha256 mismatch (actual hash: "
            "b5a2c96250612366ea272ffac6d9744aaf4b45aacd96aa7cfcb931ee3b558259)");
    }

    // Payload contains semantically incorrect JSON
    for (
        auto& payload : {
            R"j(null)j",
            R"j(1)j",
            R"j("string")j",
            R"j([])j",
            R"j({})j",
            R"j({"size":null})j",
            R"j({"size":"string"})j",
            R"j({"size":[]})j",
            R"j({"size":{}})j",
            R"j({"size":42})j", // incorrect size
            R"j({"size":5})j",
            R"j({"size":5,"sha256":null})j",
            R"j({"size":5,"sha256":1})j",
            R"j({"size":5,"sha256":[]})j",
            R"j({"size":5,"sha256":{}})j",
            R"j({"size":5,"sha256":"incorrect"})j",
            // Note: sha256("dummy") -> "b5a2c96250612366ea272ffac6d9744aaf4b45aacd96aa7cfcb931ee3b558259"
            R"j({"size":5,"sha256":"b5a2c96250612366ea272ffac6d9744aaf4b45aacd96aa7cfcb931ee3b558259"})j",
            R"j({"size":5,"sha256":"b5a2c96250612366ea272ffac6d9744aaf4b45aacd96aa7cfcb931ee3b558259","iat":null})j",
            R"j({"size":5,"sha256":"b5a2c96250612366ea272ffac6d9744aaf4b45aacd96aa7cfcb931ee3b558259","iat":"string"})j",
            R"j({"size":5,"sha256":"b5a2c96250612366ea272ffac6d9744aaf4b45aacd96aa7cfcb931ee3b558259","iat":[]})j",
            R"j({"size":5,"sha256":"b5a2c96250612366ea272ffac6d9744aaf4b45aacd96aa7cfcb931ee3b558259","iat":{}})j",
            R"j({"size":5,"sha256":"b5a2c96250612366ea272ffac6d9744aaf4b45aacd96aa7cfcb931ee3b558259","iat":0})j",
            R"j({"size":5,"sha256":"b5a2c96250612366ea272ffac6d9744aaf4b45aacd96aa7cfcb931ee3b558259","iat":0,"exp":null})j",
            R"j({"size":5,"sha256":"b5a2c96250612366ea272ffac6d9744aaf4b45aacd96aa7cfcb931ee3b558259","iat":0,"exp":"string"})j",
            R"j({"size":5,"sha256":"b5a2c96250612366ea272ffac6d9744aaf4b45aacd96aa7cfcb931ee3b558259","iat":0,"exp":[]})j",
            R"j({"size":5,"sha256":"b5a2c96250612366ea272ffac6d9744aaf4b45aacd96aa7cfcb931ee3b558259","iat":0,"exp":{}})j",
            R"j({"size":5,"sha256":"b5a2c96250612366ea272ffac6d9744aaf4b45aacd96aa7cfcb931ee3b558259","iat":0,"exp":0})j",
        })
    {
        okResponse.body = "dummy";
        okResponse.headers["X-Content-Signature"] = generateJwtFromPayload(payload);
        auto mockHttpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
        EXPECT_CALL(*mockHttpRequester, post(_)).WillOnce(Return(okResponse));

        auto verifier = std::make_unique<StrictMock<MockSignatureVerifierWrapper>>();
        EXPECT_CALL(*verifier, Verify(_, _));

        SulDownloader::SDDS3::SusRequester susRequester(mockHttpRequester, std::move(verifier));
        auto response = susRequester.request(requestParameters);
        EXPECT_FALSE(response.success);
        EXPECT_THAT(
            response.error,
            AnyOf(
                StartsWith("Failed to parse SUS response: Failed to parse SUS response with error: "
                           "[json.exception.parse_error.101]"),
                Eq("Failed to verify SUS response: Malformed SUS response: size mismatch (actual size: 5)"),
                Eq("Failed to verify SUS response: Malformed SUS response: sha256 mismatch (actual hash: "
                   "b5a2c96250612366ea272ffac6d9744aaf4b45aacd96aa7cfcb931ee3b558259)")));
    }

    // TODO LINUXDAR-7772: timestamp verification
}

TEST_F(SusRequesterTest, ValidJwtButSignaturesFailToBeVerified)
{
    auto mockHttpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    auto okResponse = Common::HttpRequests::Response();
    okResponse.status = Common::HttpRequests::HTTP_STATUS_OK;
    okResponse.body = R"({"suites": ["suite1", "suite2"], "release-groups": ["group1", "group2"]})";
    okResponse.headers["X-Content-Signature"] = generateJwt(okResponse.body);
    okResponse.errorCode = Common::HttpRequests::ResponseErrorCode::OK;
    EXPECT_CALL(*mockHttpRequester, post(_)).WillOnce(Return(okResponse));
    EXPECT_CALL(*verifier_, Verify(_, _)).WillOnce(Throw(std::runtime_error("invalid signature")));

    SulDownloader::SDDS3::SusRequester susRequester(mockHttpRequester, std::move(verifier_));
    SulDownloader::SUSRequestParameters requestParameters;

    auto response = susRequester.request(requestParameters);
    EXPECT_FALSE(response.success);
    EXPECT_EQ(response.error, "Failed to verify JWT in SUS response: invalid signature");
}

TEST_F(SusRequesterTest, writesToIniProxyInfoWhenProxyParameterProvided)
{
    const std::string expectedOutputToFile = "usedProxy = true\nusedUpdateCache = false\nusedMessageRelay = true\nproxyOrMessageRelayURL = theurl\n";

    auto mockHttpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    EXPECT_CALL(*mockHttpRequester, post(_)).Times(1);
    EXPECT_CALL(*verifier_, Verify(_, _)).Times(0);

    EXPECT_CALL(*mockFileSystem_, writeFile(_,expectedOutputToFile)).Times(1);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };

    auto mockSystemUtils = std::make_unique<StrictMock<MockSystemUtils>>();
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable(_)).WillOnce(Return("imnotempty"));
    Tests::ScopedReplaceSystemUtils scopedSystemUtils (std::move(mockSystemUtils));

    SulDownloader::SDDS3::SusRequester susRequester(mockHttpRequester, std::move(verifier_));
    SulDownloader::SUSRequestParameters requestParameters;
    Common::Policy::Proxy proxy ("theurl");
    requestParameters.proxy = proxy;
    auto response = susRequester.request(requestParameters);
    ASSERT_FALSE(response.success);
    ASSERT_TRUE(response.data.releaseGroups.empty());
    ASSERT_TRUE(response.data.suites.empty());
}

TEST_F(SusRequesterTest, writesToIniNoProxyInfoWhenNoProxyParameter)
{
    const std::string expectedOutputToFile = "usedProxy = false\nusedUpdateCache = false\nusedMessageRelay = false\nproxyOrMessageRelayURL = \n";

    auto mockHttpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    EXPECT_CALL(*mockHttpRequester, post(_)).Times(1);
    EXPECT_CALL(*verifier_, Verify(_, _)).Times(0);

    EXPECT_CALL(*mockFileSystem_, writeFile(_,expectedOutputToFile)).Times(1);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };

    auto mockSystemUtils = std::make_unique<StrictMock<MockSystemUtils>>();
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable(_)).WillOnce(Return("imnotempty"));
    Tests::ScopedReplaceSystemUtils scopedSystemUtils (std::move(mockSystemUtils));

    SulDownloader::SDDS3::SusRequester susRequester(mockHttpRequester, std::move(verifier_));
    SulDownloader::SUSRequestParameters requestParameters;
    auto response = susRequester.request(requestParameters);
    ASSERT_FALSE(response.success);
    ASSERT_TRUE(response.data.releaseGroups.empty());
    ASSERT_TRUE(response.data.suites.empty());
}