// Copyright 2022-2023 Sophos Limited. All rights reserved.
/*
 * This test binary runs against the test server HttpTestServer during TAP test.
 * The tests can also be run locally by running HttpTestServer.py (right click run), and then running the tests here
 * as normal.
 * The first lot of tests are parameterised into HTTP and HTTPS tests so that the same tests did not need to be
 * duplicated for both HTTP and HTTPS. The test server runs both an HTTP and HTTPS server.
 */

#include "Common/CurlWrapper/CurlWrapper.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/HttpRequestsImpl/HttpRequesterImpl.h"
#include "tests/Common/Helpers/LogInitializedTests.h"

#include <gtest/gtest.h>

namespace
{
    struct HttpTestParam
    {
        std::string url;
        std::string urlAndPort;
        int port = 0;
        std::string certToUse = "";
        std::string proxyToUse = "";
        std::string proxyUsernameToUse = "";
        std::string proxyPasswordToUse = "";
    };

    // Test helper function to make creating the different requests for each parameterised run easier
    Common::HttpRequests::RequestConfig getTestRequest(HttpTestParam testParams)
    {
        Common::HttpRequests::RequestConfig request = { .url = testParams.url };
        if (!testParams.certToUse.empty())
        {
            request.certPath = testParams.certToUse;
        }
        if (!testParams.proxyToUse.empty())
        {
            request.proxy = testParams.proxyToUse;
        }
        if (!testParams.proxyUsernameToUse.empty())
        {
            request.proxyUsername = testParams.proxyUsernameToUse;
        }
        if (!testParams.proxyPasswordToUse.empty())
        {
            request.proxyPassword = testParams.proxyPasswordToUse;
        }
        return request;
    }

    // Helper function to allow us to  use the certs both in TAP and when run locally
    std::string getCertLocation()
    {
        static std::string certPath = "";
        if (certPath.empty())
        {
            std::string certName = "localhost-selfsigned.crt";
            std::string srcDirCertPath =
                Common::FileSystem::join(Common::FileSystem::dirName(std::string(__FILE__)), certName);
            if (Common::FileSystem::fileSystem()->isFile(srcDirCertPath))
            {
                certPath = srcDirCertPath;
            }
            else
            {
                std::optional<std::string> exe = Common::FileSystem::fileSystem()->readlink("/proc/self/exe");
                std::string exeDirCertPath =
                    Common::FileSystem::join(Common::FileSystem::dirName(exe.value()), certName);
                if (Common::FileSystem::fileSystem()->isFile(exeDirCertPath))
                {
                    certPath = exeDirCertPath;
                }
                else
                {
                    std::stringstream ss;
                    ss << "Live network tests cannot find the test cert here: " << srcDirCertPath
                       << ", or here: " << exeDirCertPath;
                    throw std::runtime_error(ss.str());
                }
            }
        }
        return certPath;
    }
} // namespace

class HttpRequesterLiveNetworkTestsParam : public ::testing::TestWithParam<HttpTestParam>
{
    Common::Logging::ConsoleLoggingSetup m_loggingSetup;

public:
    std::vector<std::string> m_filesToRemove;

protected:
    virtual void SetUp()
    {
        // Give the server a chance to keep up
        usleep(200000);
    }

    virtual void TearDown()
    {
        auto fs = Common::FileSystem::fileSystem();
        for (const auto& file : m_filesToRemove)
        {
            std::cout << "Cleaning up test file: " << file << std::endl;
            try
            {
                fs->removeFile(file);
            }
            catch (const std::exception& ex)
            {
                // don't care
            }
        }
    }
};

INSTANTIATE_TEST_SUITE_P(
    LiveNetworkTestRuns,
    HttpRequesterLiveNetworkTestsParam,
    ::testing::Values(
        HttpTestParam{ .url = "http://localhost", .urlAndPort = "http://localhost:7780", .port = 7780 },
        HttpTestParam{ .url = "https://localhost",
                       .urlAndPort = "https://localhost:7743",
                       .port = 7743,
                       .certToUse = getCertLocation() },
        HttpTestParam{ .url = "http://localhost",
                       .urlAndPort = "http://localhost:7780",
                       .port = 7780,
                       .proxyToUse = "localhost:7750" },
        HttpTestParam{ .url = "http://localhost",
                       .urlAndPort = "http://localhost:7780",
                       .port = 7780,
                       .proxyToUse = "localhost:7751",
                       .proxyUsernameToUse="user",
                       .proxyPasswordToUse="password"}
    )
);


TEST_P(HttpRequesterLiveNetworkTestsParam, getWithPort)
{
    std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
        std::make_shared<Common::CurlWrapper::CurlWrapper>();
    Common::HttpRequestsImpl::HttpRequesterImpl client = Common::HttpRequestsImpl::HttpRequesterImpl(curlWrapper);

    HttpTestParam param = GetParam();
    Common::HttpRequests::RequestConfig request = getTestRequest(param);

    std::string testUrlResource = "getWithPort";
    request.url = param.urlAndPort + "/" + testUrlResource;;
    Common::HttpRequests::Response response = client.get(request);

    ASSERT_EQ(response.status, 200);
    std::string expected_response = "/" + testUrlResource + " response body";
    ASSERT_EQ(response.body, expected_response);
    ASSERT_EQ(response.headers.count("test_header"), 1);
    ASSERT_EQ(response.headers.at("test_header"), "test_header_value");
}

TEST_P(HttpRequesterLiveNetworkTestsParam, getWithPortAndHeaders)
{
    HttpTestParam param = GetParam();
    Common::HttpRequests::RequestConfig request = getTestRequest(param);
    std::string testUrlResource = "getWithPortAndHeaders";
    std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
        std::make_shared<Common::CurlWrapper::CurlWrapper>();
    Common::HttpRequestsImpl::HttpRequesterImpl client = Common::HttpRequestsImpl::HttpRequesterImpl(curlWrapper);

    request.url = param.urlAndPort + "/" + testUrlResource;
    request.headers = Common::HttpRequests::Headers{ { "req_header", "a value" } };
    Common::HttpRequests::Response response = client.get(request);

    ASSERT_EQ(response.status, 200);
    std::string expected_response = "/" + testUrlResource + " response body";
    ASSERT_EQ(response.body, expected_response);
}

TEST_P(HttpRequesterLiveNetworkTestsParam, getWithPortAndParameters)
{
    HttpTestParam param = GetParam();
    Common::HttpRequests::RequestConfig request = getTestRequest(param);
    std::string testUrlResource = "getWithPortAndParameters";
    std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
        std::make_shared<Common::CurlWrapper::CurlWrapper>();
    Common::HttpRequestsImpl::HttpRequesterImpl client = Common::HttpRequestsImpl::HttpRequesterImpl(curlWrapper);

    request.url = param.urlAndPort + "/" + testUrlResource;
    request.parameters = Common::HttpRequests::Parameters{ { "param1", "value1" } };
    Common::HttpRequests::Response response = client.get(request);

    ASSERT_EQ(response.status, 200);
    std::string expected_response = "/" + testUrlResource + " response body";
    ASSERT_EQ(response.body, expected_response);
}

TEST_P(HttpRequesterLiveNetworkTestsParam, getWithFileDownloadAndExplicitFileName)
{
    HttpTestParam param = GetParam();
    Common::HttpRequests::RequestConfig request = getTestRequest(param);
    std::string testUrlResource = "getWithFileDownloadAndExplicitFileName";

    std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
        std::make_shared<Common::CurlWrapper::CurlWrapper>();
    Common::HttpRequestsImpl::HttpRequesterImpl client = Common::HttpRequestsImpl::HttpRequesterImpl(curlWrapper);

    std::string fileDownloadLocation = "/tmp/" + testUrlResource + ".txt";
    m_filesToRemove.push_back(fileDownloadLocation);
    std::string fileExpectedContent = "Sample text file to use in HTTP tests.";

    request.url = param.urlAndPort + "/" + testUrlResource;
    request.fileDownloadLocation = fileDownloadLocation;
    Common::HttpRequests::Response response = client.get(request);

    auto fs = Common::FileSystem::fileSystem();
    ASSERT_TRUE(fs->exists(fileDownloadLocation));
    ASSERT_EQ(fs->readFile(fileDownloadLocation), fileExpectedContent);
    ASSERT_EQ(response.status, 200);

    // Downloaded to file so body will be empty
    ASSERT_EQ(response.body, "");
}

TEST_P(HttpRequesterLiveNetworkTestsParam, getWithFileDownloadToDirectory)
{
    HttpTestParam param = GetParam();
    Common::HttpRequests::RequestConfig request = getTestRequest(param);
    std::string testUrlResource = "getWithFileDownloadToDirectory";

    std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
        std::make_shared<Common::CurlWrapper::CurlWrapper>();
    Common::HttpRequestsImpl::HttpRequesterImpl client = Common::HttpRequestsImpl::HttpRequesterImpl(curlWrapper);

    std::string fileDownloadLocationDir = "/tmp";
    std::string fileDownloadLocationFullPath = fileDownloadLocationDir + "/sample.txt";
    m_filesToRemove.push_back(fileDownloadLocationFullPath);
    std::string fileExpectedContent = "Sample text file to use in HTTP tests.";

    request.url = param.urlAndPort + "/" + testUrlResource + "/sample.txt";
    request.fileDownloadLocation = fileDownloadLocationDir;
    Common::HttpRequests::Response response = client.get(request);

    auto fs = Common::FileSystem::fileSystem();
    ASSERT_TRUE(fs->exists(fileDownloadLocationFullPath));
    ASSERT_EQ(fs->readFile(fileDownloadLocationFullPath), fileExpectedContent);
    ASSERT_EQ(response.status, 200);

    // Downloaded to file, body will be empty
    ASSERT_EQ(response.body, "");
}

TEST_P(HttpRequesterLiveNetworkTestsParam, getWithFileDownloadToDirectoryAndContentDispositionHeader)
{
    HttpTestParam param = GetParam();
    Common::HttpRequests::RequestConfig request = getTestRequest(param);
    std::string testUrlResource = "getWithFileDownloadToDirectoryAndContentDispositionHeader";

    std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
        std::make_shared<Common::CurlWrapper::CurlWrapper>();
    Common::HttpRequestsImpl::HttpRequesterImpl client = Common::HttpRequestsImpl::HttpRequesterImpl(curlWrapper);

    std::string fileDownloadLocationDir = "/tmp";
    std::string fileDownloadLocationFullPath = fileDownloadLocationDir + "/differentName.txt";
    m_filesToRemove.push_back(fileDownloadLocationFullPath);
    std::string fileExpectedContent = "Sample text file to use in HTTP tests.";

    request.url = param.urlAndPort + "/" + testUrlResource + "/sample.txt";
    request.fileDownloadLocation = fileDownloadLocationDir;
    Common::HttpRequests::Response response = client.get(request);

    auto fs = Common::FileSystem::fileSystem();
    ASSERT_TRUE(fs->exists(fileDownloadLocationFullPath));
    ASSERT_EQ(fs->readFile(fileDownloadLocationFullPath), fileExpectedContent);
    ASSERT_EQ(response.status, 200);

    // Downloaded to file so body will be empty
    ASSERT_EQ(response.body, "");
}

TEST_P(HttpRequesterLiveNetworkTestsParam, getWithPortAndTimeout)
{
    HttpTestParam param = GetParam();
    Common::HttpRequests::RequestConfig request = getTestRequest(param);
    std::string testUrlResource = "getWithPortAndTimeout";
    std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
        std::make_shared<Common::CurlWrapper::CurlWrapper>();
    Common::HttpRequestsImpl::HttpRequesterImpl client = Common::HttpRequestsImpl::HttpRequesterImpl(curlWrapper);

    request.url = param.urlAndPort + "/" + testUrlResource;
    request.timeout = 3;
    Common::HttpRequests::Response response = client.get(request);

    ASSERT_EQ(response.status, Common::HttpRequests::HTTP_STATUS_NOT_SET);
    ASSERT_EQ(response.body, "");
    ASSERT_EQ(response.errorCode, Common::HttpRequests::ResponseErrorCode::TIMEOUT);
    ASSERT_EQ(response.error, "Timeout was reached");
}

// Due to changes in Curl 7.87.0, rate limiting is more about the long-term average, so short bursts can go over the
// set limit.
// So to be able to test it we would need to send more data, at least 1-10MB.
// This will fill up the Robot logs and requires adapting the test server to use HTTP/1.1 (> frame of data over proxy).
// Since we don't use bandwidth limiting in any code, I decided to disable this test.
TEST_P(HttpRequesterLiveNetworkTestsParam, DISABLED_getWithPortAndBandwidthLimit)
{
    HttpTestParam param = GetParam();
    Common::HttpRequests::RequestConfig request = getTestRequest(param);
    std::string testUrlResource = "getWithPortAndBandwidthLimit";

    std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
        std::make_shared<Common::CurlWrapper::CurlWrapper>();
    Common::HttpRequestsImpl::HttpRequesterImpl client = Common::HttpRequestsImpl::HttpRequesterImpl(curlWrapper);

    request.url = param.urlAndPort + "/" + testUrlResource;
    request.bandwidthLimit = 100;
    auto start = std::chrono::high_resolution_clock::now();

    // Download a 1000B file at ~100B/s per second
    Common::HttpRequests::Response response = client.get(request);

    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(stop - start);

    // Bandwidth limiting is not perfect so even though in theory this should take 20 seconds,
    // if it took more than a few seconds (compared to 10 milliseconds that it normally takes) then
    // it's close enough and we don't want this to be a flaky test
    ASSERT_GE(duration.count(), 3);
    ASSERT_EQ(response.status, 200);
    std::string expected_response = std::string(1000, 'a');
    ASSERT_EQ(response.body, expected_response);
}

// PUT Tests

TEST_P(HttpRequesterLiveNetworkTestsParam, putWithPort)
{
    HttpTestParam param = GetParam();
    Common::HttpRequests::RequestConfig request = getTestRequest(param);
    std::string testUrlResource = "putWithPort";
    std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
        std::make_shared<Common::CurlWrapper::CurlWrapper>();
    Common::HttpRequestsImpl::HttpRequesterImpl client = Common::HttpRequestsImpl::HttpRequesterImpl(curlWrapper);

    request.url = param.urlAndPort + "/" + testUrlResource;
    Common::HttpRequests::Response response = client.put(request);

    ASSERT_EQ(response.status, 200);
    std::string expected_response = "/" + testUrlResource + " response body";
    ASSERT_EQ(response.body, expected_response);
    ASSERT_EQ(response.headers.count("test_header"), 1);
    ASSERT_EQ(response.headers.at("test_header"), "test_header_value");
}

TEST_P(HttpRequesterLiveNetworkTestsParam, putWithFileUpload)
{
    HttpTestParam param = GetParam();
    Common::HttpRequests::RequestConfig request = getTestRequest(param);
    std::string testUrlResource = "putWithFileUpload";

    m_filesToRemove.emplace_back("/tmp/testPutFile");
    std::ignore = system("bash -c 'echo test > /tmp/testPutFile'");
    std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
        std::make_shared<Common::CurlWrapper::CurlWrapper>();
    Common::HttpRequestsImpl::HttpRequesterImpl client = Common::HttpRequestsImpl::HttpRequesterImpl(curlWrapper);

    request.url = param.urlAndPort + "/" + testUrlResource;
    // "test" is 4 bytes.
    request.headers = Common::HttpRequests::Headers{ { "Content-Length", "4" } };
    request.fileToUpload = "/tmp/testPutFile";

    Common::HttpRequests::Response response = client.put(request);

    ASSERT_EQ(response.status, 200);
    std::string expected_response = "/" + testUrlResource + " response body, you PUT 4 bytes";
    ASSERT_EQ(response.body, expected_response);
    ASSERT_EQ(response.headers.count("test_header"), 1);
    ASSERT_EQ(response.headers.at("test_header"), "test_header_value");
}

// POST Tests

TEST_P(HttpRequesterLiveNetworkTestsParam, postWithPort)
{
    HttpTestParam param = GetParam();
    Common::HttpRequests::RequestConfig request = getTestRequest(param);
    std::string testUrlResource = "postWithPort";
    std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
        std::make_shared<Common::CurlWrapper::CurlWrapper>();
    Common::HttpRequestsImpl::HttpRequesterImpl client = Common::HttpRequestsImpl::HttpRequesterImpl(curlWrapper);

    request.url = param.urlAndPort + "/" + testUrlResource;
    Common::HttpRequests::Response response = client.post(request);

    ASSERT_EQ(response.status, 200);
    std::string expected_response = "/" + testUrlResource + " response body";
    ASSERT_EQ(response.body, expected_response);
}

TEST_P(HttpRequesterLiveNetworkTestsParam, postWithData)
{
    HttpTestParam param = GetParam();
    Common::HttpRequests::RequestConfig request = getTestRequest(param);
    std::string testUrlResource = "postWithData";
    std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
        std::make_shared<Common::CurlWrapper::CurlWrapper>();
    Common::HttpRequestsImpl::HttpRequesterImpl client = Common::HttpRequestsImpl::HttpRequesterImpl(curlWrapper);

    request.url = param.urlAndPort + "/" + testUrlResource;
    request.data = "SamplePostData";

    Common::HttpRequests::Response response = client.post(request);

    ASSERT_EQ(response.status, 200);
    std::string expected_response = "/" + testUrlResource + " response body";
    ASSERT_EQ(response.body, expected_response);
}


// DELETE tests

TEST_P(HttpRequesterLiveNetworkTestsParam, deleteWithPort)
{
    std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
        std::make_shared<Common::CurlWrapper::CurlWrapper>();
    Common::HttpRequestsImpl::HttpRequesterImpl client = Common::HttpRequestsImpl::HttpRequesterImpl(curlWrapper);

    HttpTestParam param = GetParam();
    Common::HttpRequests::RequestConfig request = getTestRequest(param);

    std::string testUrlResource = "deleteWithPort";
    request.url = param.urlAndPort + "/" + testUrlResource;;
    Common::HttpRequests::Response response = client.del(request);

    ASSERT_EQ(response.status, 200);
    std::string expected_response = "/" + testUrlResource + " response body";
    ASSERT_EQ(response.body, expected_response);
    ASSERT_EQ(response.headers.count("test_header"), 1);
    ASSERT_EQ(response.headers.at("test_header"), "test_header_value");
}


// OPTIONS tests

TEST_P(HttpRequesterLiveNetworkTestsParam, optionsWithPort)
{
    std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
        std::make_shared<Common::CurlWrapper::CurlWrapper>();
    Common::HttpRequestsImpl::HttpRequesterImpl client = Common::HttpRequestsImpl::HttpRequesterImpl(curlWrapper);

    HttpTestParam param = GetParam();
    Common::HttpRequests::RequestConfig request = getTestRequest(param);

    std::string testUrlResource = "optionsWithPort";
    request.url = param.urlAndPort + "/" + testUrlResource;;
    Common::HttpRequests::Response response = client.options(request);

    ASSERT_EQ(response.status, 200);
    std::string expected_response = "/" + testUrlResource + " response body";
    ASSERT_EQ(response.body, expected_response);
    ASSERT_EQ(response.headers.count("test_header"), 1);
    ASSERT_EQ(response.headers.at("test_header"), "test_header_value");
}

// Some live network tests that do not need to be parameterised into HTTP/HTTPS tests.

class HttpRequesterLiveNetworkTests : public LogInitializedTests
{
};

const int HTTP_PORT = 7780;
const std::string HTTP_URL_WITH_PORT = "http://localhost:" + std::to_string(HTTP_PORT);

const int HTTPS_PORT = 7743;
const std::string HTTPS_URL_WITH_PORT = "https://localhost:" + std::to_string(HTTPS_PORT);

TEST_F(HttpRequesterLiveNetworkTests, httpsFailsWithMissingCert)
{
    std::string testName = ::testing::UnitTest::GetInstance()->current_test_info()->name();
    std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
        std::make_shared<Common::CurlWrapper::CurlWrapper>();
    Common::HttpRequestsImpl::HttpRequesterImpl client = Common::HttpRequestsImpl::HttpRequesterImpl(curlWrapper);
    std::string url = HTTPS_URL_WITH_PORT + "/" + testName;
    Common::HttpRequests::Response response =
        client.get(Common::HttpRequests::RequestConfig{ .url = url, .certPath = "/missing/file.crt" });

    ASSERT_EQ(response.status, Common::HttpRequests::HTTP_STATUS_NOT_SET);
    ASSERT_EQ(response.body, "");
    ASSERT_EQ(response.errorCode, Common::HttpRequests::ResponseErrorCode::CERTIFICATE_ERROR);
}

TEST_F(HttpRequesterLiveNetworkTests, httpFailsWith404)
{
    std::string testName = ::testing::UnitTest::GetInstance()->current_test_info()->name();
    std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
        std::make_shared<Common::CurlWrapper::CurlWrapper>();
    Common::HttpRequestsImpl::HttpRequesterImpl client = Common::HttpRequestsImpl::HttpRequesterImpl(curlWrapper);

    std::string url = HTTP_URL_WITH_PORT + "/somethingthatdoesnotexist";
    Common::HttpRequests::Response response =
        client.get(Common::HttpRequests::RequestConfig{ .url = url });

    ASSERT_EQ(response.status, 404);
    ASSERT_EQ(response.body, "Not a valid test case!");
    ASSERT_EQ(response.errorCode, Common::HttpRequests::ResponseErrorCode::OK);
}

