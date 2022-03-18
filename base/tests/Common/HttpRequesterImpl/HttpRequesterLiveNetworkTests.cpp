/*
 * This test binary runs against the test server HttpTestServer during TAP test.
 * The tests can also be run locally by running HttpTestServer.py (right click run), and then running the tests here
 * as normal.
 * The first lot of tests are parameterised into HTTP and HTTPS tests so that the same tests did not need to be
 * duplicated for both HTTP and HTTPS. The test server runs both an HTTP and HTTPS server.
 */

#include "../Helpers/LogInitializedTests.h"
#include "FileSystem/IFileSystem.h"

#include <Common/CurlWrapper/CurlWrapper.h>
#include <Common/HttpRequestsImpl/HttpRequesterImpl.h>
#include <curl/curl.h>
#include <gtest/gtest.h>

namespace
{
    struct HttpTestParam
    {
        std::string url;
        std::string urlAndPort;
        int port = 0;
        std::string certToUse = "";
    };

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
    virtual void SetUp() {}

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

INSTANTIATE_TEST_CASE_P(
    LeapYearTests,
    HttpRequesterLiveNetworkTestsParam,
    ::testing::Values(
        HttpTestParam{ .url = "http://localhost", .urlAndPort = "http://localhost:7780", .port = 7780 },
        HttpTestParam{ .url = "https://localhost",
                       .urlAndPort = "https://localhost:7743",
                       .port = 7743,
                       .certToUse = getCertLocation() }));

TEST_P(HttpRequesterLiveNetworkTestsParam, getWithPort)
{
    HttpTestParam param = GetParam();
    std::string testUrlResource = "getWithPort";
    std::string url = param.urlAndPort + "/" + testUrlResource;

    std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
        std::make_shared<Common::CurlWrapper::CurlWrapper>();
    Common::HttpRequestsImpl::HttpRequesterImpl client = Common::HttpRequestsImpl::HttpRequesterImpl(curlWrapper);
    Common::HttpRequests::RequestConfig request = { .url = url };
    if (!param.certToUse.empty())
    {
        request.certPath = param.certToUse;
    }
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
    std::string testUrlResource = "getWithPortAndHeaders";
    std::string url = param.urlAndPort + "/" + testUrlResource;
    std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
        std::make_shared<Common::CurlWrapper::CurlWrapper>();
    Common::HttpRequestsImpl::HttpRequesterImpl client = Common::HttpRequestsImpl::HttpRequesterImpl(curlWrapper);

    Common::HttpRequests::RequestConfig request = { .url = url };
    if (!param.certToUse.empty())
    {
        request.certPath = param.certToUse;
    }

    request.headers = Common::HttpRequests::Headers{ { "req_header", "a value" } };
    Common::HttpRequests::Response response = client.get(request);

    ASSERT_EQ(response.status, 200);
    std::string expected_response = "/" + testUrlResource + " response body";
    ASSERT_EQ(response.body, expected_response);
}

TEST_P(HttpRequesterLiveNetworkTestsParam, getWithPortAndParameters)
{
    HttpTestParam param = GetParam();
    std::string testUrlResource = "getWithPortAndParameters";
    std::string url = param.urlAndPort + "/" + testUrlResource;
    std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
        std::make_shared<Common::CurlWrapper::CurlWrapper>();
    Common::HttpRequestsImpl::HttpRequesterImpl client = Common::HttpRequestsImpl::HttpRequesterImpl(curlWrapper);

    Common::HttpRequests::RequestConfig request = { .url = url };
    if (!param.certToUse.empty())
    {
        request.certPath = param.certToUse;
    }

    request.parameters = Common::HttpRequests::Parameters{ { "param1", "value1" } };
    Common::HttpRequests::Response response = client.get(request);

    ASSERT_EQ(response.status, 200);
    std::string expected_response = "/" + testUrlResource + " response body";
    ASSERT_EQ(response.body, expected_response);
}

TEST_P(HttpRequesterLiveNetworkTestsParam, getWithFileDownloadAndExplicitFileName)
{
    HttpTestParam param = GetParam();
    std::string testUrlResource = "getWithFileDownloadAndExplicitFileName";
    std::string url = param.urlAndPort + "/" + testUrlResource;

    std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
        std::make_shared<Common::CurlWrapper::CurlWrapper>();
    Common::HttpRequestsImpl::HttpRequesterImpl client = Common::HttpRequestsImpl::HttpRequesterImpl(curlWrapper);

    std::string fileDownloadLocation = "/tmp/" + testUrlResource + ".txt";
    m_filesToRemove.push_back(fileDownloadLocation);
    std::string fileExpectedContent = "Sample text file to use in HTTP tests.";

    Common::HttpRequests::RequestConfig request = { .url = url };
    if (!param.certToUse.empty())
    {
        request.certPath = param.certToUse;
    }

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
    std::string testUrlResource = "getWithFileDownloadToDirectory";
    std::string url = param.urlAndPort + "/" + testUrlResource + "/sample.txt";

    std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
        std::make_shared<Common::CurlWrapper::CurlWrapper>();
    Common::HttpRequestsImpl::HttpRequesterImpl client = Common::HttpRequestsImpl::HttpRequesterImpl(curlWrapper);

    std::string fileDownloadLocationDir = "/tmp";
    std::string fileDownloadLocationFullPath = fileDownloadLocationDir + "/sample.txt";
    m_filesToRemove.push_back(fileDownloadLocationFullPath);
    std::string fileExpectedContent = "Sample text file to use in HTTP tests.";

    Common::HttpRequests::RequestConfig request = { .url = url };
    if (!param.certToUse.empty())
    {
        request.certPath = param.certToUse;
    }

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
    std::string testUrlResource = "getWithFileDownloadToDirectoryAndContentDispositionHeader";
    std::string url = param.urlAndPort + "/" + testUrlResource + "/sample.txt";

    std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
        std::make_shared<Common::CurlWrapper::CurlWrapper>();
    Common::HttpRequestsImpl::HttpRequesterImpl client = Common::HttpRequestsImpl::HttpRequesterImpl(curlWrapper);

    std::string fileDownloadLocationDir = "/tmp";
    std::string fileDownloadLocationFullPath = fileDownloadLocationDir + "/differentName.txt";
    m_filesToRemove.push_back(fileDownloadLocationFullPath);
    std::string fileExpectedContent = "Sample text file to use in HTTP tests.";

    Common::HttpRequests::RequestConfig request = { .url = url };
    if (!param.certToUse.empty())
    {
        request.certPath = param.certToUse;
    }

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
    std::string testUrlResource = "getWithPortAndTimeout";
    std::string url = param.urlAndPort + "/" + testUrlResource;

    std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
        std::make_shared<Common::CurlWrapper::CurlWrapper>();
    Common::HttpRequestsImpl::HttpRequesterImpl client = Common::HttpRequestsImpl::HttpRequesterImpl(curlWrapper);

    Common::HttpRequests::RequestConfig request = { .url = url };
    if (!param.certToUse.empty())
    {
        request.certPath = param.certToUse;
    }
    request.timeout = 3;
    Common::HttpRequests::Response response = client.get(request);

    ASSERT_EQ(response.status, Common::HttpRequests::STATUS_NOT_SET);
    ASSERT_EQ(response.body, "");
    ASSERT_EQ(response.errorCode, Common::HttpRequests::ResponseErrorCode::TIMEOUT);
    ASSERT_EQ(response.error, "Timeout was reached");
}

TEST_P(HttpRequesterLiveNetworkTestsParam, getWithPortAndBandwidthLimit)
{
    HttpTestParam param = GetParam();
    std::string testUrlResource = "getWithPortAndBandwidthLimit";
    std::string url = param.urlAndPort + "/" + testUrlResource;

    std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
        std::make_shared<Common::CurlWrapper::CurlWrapper>();
    Common::HttpRequestsImpl::HttpRequesterImpl client = Common::HttpRequestsImpl::HttpRequesterImpl(curlWrapper);

    Common::HttpRequests::RequestConfig request = { .url = url };
    if (!param.certToUse.empty())
    {
        request.certPath = param.certToUse;
    }
    request.bandwidthLimit = 100;
    auto start = std::chrono::high_resolution_clock::now();

    // Download a 1000B file at ~100B/s per second
    Common::HttpRequests::Response response = client.get(request);

    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(stop - start);

    // Bandwidth limiting is not perfect so even though in theory this should take 10 seconds, if it took more than 8
    // it's close enough and we don't want this to be a flaky test
    ASSERT_GT(duration.count(), 8);
    ASSERT_EQ(response.status, 200);
    std::string expected_response = std::string(1000, 'a');
    ASSERT_EQ(response.body, expected_response);
}

// PUT Tests

TEST_P(HttpRequesterLiveNetworkTestsParam, putWithPort)
{
    HttpTestParam param = GetParam();
    std::string testUrlResource = "putWithPort";
    std::string url = param.urlAndPort + "/" + testUrlResource;

    std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
        std::make_shared<Common::CurlWrapper::CurlWrapper>();
    Common::HttpRequestsImpl::HttpRequesterImpl client = Common::HttpRequestsImpl::HttpRequesterImpl(curlWrapper);

    Common::HttpRequests::RequestConfig request = { .url = url };
    if (!param.certToUse.empty())
    {
        request.certPath = param.certToUse;
    }

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
    std::string testUrlResource = "putWithFileUpload";
    std::string url = param.urlAndPort + "/" + testUrlResource;

    m_filesToRemove.emplace_back("/tmp/testPutFile");
    system("bash -c 'echo test > /tmp/testPutFile'");
    // "test" is 4 bytes.
    std::string fileSize = "4";
    std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
        std::make_shared<Common::CurlWrapper::CurlWrapper>();
    Common::HttpRequestsImpl::HttpRequesterImpl client = Common::HttpRequestsImpl::HttpRequesterImpl(curlWrapper);

    Common::HttpRequests::RequestConfig request = { .url = url };
    if (!param.certToUse.empty())
    {
        request.certPath = param.certToUse;
    }
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
    std::string testUrlResource = "postWithPort";
    std::string url = param.urlAndPort + "/" + testUrlResource;

    std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
        std::make_shared<Common::CurlWrapper::CurlWrapper>();
    Common::HttpRequestsImpl::HttpRequesterImpl client = Common::HttpRequestsImpl::HttpRequesterImpl(curlWrapper);

    Common::HttpRequests::RequestConfig request = { .url = url };
    if (!param.certToUse.empty())
    {
        request.certPath = param.certToUse;
    }

    Common::HttpRequests::Response response = client.post(request);

    ASSERT_EQ(response.status, 200);
    std::string expected_response = "/" + testUrlResource + " response body";
    ASSERT_EQ(response.body, expected_response);
}

TEST_P(HttpRequesterLiveNetworkTestsParam, postWithData)
{
    HttpTestParam param = GetParam();
    std::string testUrlResource = "postWithData";
    std::string url = param.urlAndPort + "/" + testUrlResource;

    std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
        std::make_shared<Common::CurlWrapper::CurlWrapper>();
    Common::HttpRequestsImpl::HttpRequesterImpl client = Common::HttpRequestsImpl::HttpRequesterImpl(curlWrapper);

    Common::HttpRequests::RequestConfig request = { .url = url };
    if (!param.certToUse.empty())
    {
        request.certPath = param.certToUse;
    }
    request.data = "SamplePostData";

    Common::HttpRequests::Response response = client.post(request);

    ASSERT_EQ(response.status, 200);
    std::string expected_response = "/" + testUrlResource + " response body";
    ASSERT_EQ(response.body, expected_response);
}

// Some live network tests that do not need to be parameterised into HTTP/HTTPS tests.

class HttpRequesterLiveNetworkTests : public LogInitializedTests
{
};

const int HTTPS_PORT = 7743;
const std::string HTTPS_URL_WITH_PORT = "https://localhost:" + std::to_string(HTTPS_PORT);

TEST_F(HttpRequesterLiveNetworkTests, httpsFailsWithMissing)
{
    std::string testName = ::testing::UnitTest::GetInstance()->current_test_info()->name();
    std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
        std::make_shared<Common::CurlWrapper::CurlWrapper>();
    Common::HttpRequestsImpl::HttpRequesterImpl client = Common::HttpRequestsImpl::HttpRequesterImpl(curlWrapper);
    std::string url = HTTPS_URL_WITH_PORT + "/" + testName;
    Common::HttpRequests::Response response =
        client.get(Common::HttpRequests::RequestConfig{ .url = url, .certPath = "/missing/file.crt" });

    ASSERT_EQ(response.status, Common::HttpRequests::STATUS_NOT_SET);
    ASSERT_EQ(response.body, "");
    ASSERT_EQ(response.errorCode, Common::HttpRequests::ResponseErrorCode::CERTIFICATE_ERROR);
}
