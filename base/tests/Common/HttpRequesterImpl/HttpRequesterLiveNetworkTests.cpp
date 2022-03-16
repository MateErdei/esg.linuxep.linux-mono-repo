/*
 * This test binary runs against the test server HttpTestServer during TAP test.
 */

#include "../Helpers/LogInitializedTests.h"
#include "FileSystem/IFileSystem.h"

#include <Common/CurlWrapper/CurlWrapper.h>
#include <Common/HttpRequestsImpl/HttpRequesterImpl.h>
#include <curl/curl.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

class HttpRequesterLiveNetworkTests : public LogInitializedTests
{

public:
    std::vector<std::string> m_filesToRemove;

protected:
    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
        auto fs = Common::FileSystem::fileSystem();
        for (const auto& file : m_filesToRemove)
        {
            std::cout << "Cleaning up test file: " << file << std::endl;
            fs->removeFile(file);
        }
    }
};

// Test server port
const int PORT = 7780;
const std::string URL = "http://localhost";
const std::string URL_WITH_PORT = "http://localhost:" + std::to_string(PORT);

// GET Tests

TEST_F(HttpRequesterLiveNetworkTests, getWithPort)
{
    std::string testName = ::testing::UnitTest::GetInstance()->current_test_info()->name();
    std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
        std::make_shared<Common::CurlWrapper::CurlWrapper>();
    Common::HttpRequestsImpl::HttpRequesterImpl client = Common::HttpRequestsImpl::HttpRequesterImpl(curlWrapper);
    std::string url = URL_WITH_PORT + "/" + testName;
    Common::HttpRequests::Response response = client.get(Common::HttpRequests::RequestConfig{ .url = url });

    ASSERT_EQ(response.status, 200);
    std::string expected_response = "/" + testName + " response body";
    ASSERT_EQ(response.body, expected_response);
    ASSERT_EQ(response.headers.count("test_header"), 1);
    ASSERT_EQ(response.headers.at("test_header"), "test_header_value");
}

TEST_F(HttpRequesterLiveNetworkTests, getWithPortAndHeaders)
{
    std::string testName = ::testing::UnitTest::GetInstance()->current_test_info()->name();
    std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
        std::make_shared<Common::CurlWrapper::CurlWrapper>();
    Common::HttpRequestsImpl::HttpRequesterImpl client = Common::HttpRequestsImpl::HttpRequesterImpl(curlWrapper);
    std::string url = URL_WITH_PORT + "/" + testName;
    Common::HttpRequests::Response response = client.get(Common::HttpRequests::RequestConfig{
        .url = url, .headers = Common::HttpRequests::Headers{ { "req_header", "a value" } } });

    ASSERT_EQ(response.status, 200);
    std::string expected_response = "/" + testName + " response body";
    ASSERT_EQ(response.body, expected_response);
}

TEST_F(HttpRequesterLiveNetworkTests, getWithPortAndParameters)
{
    std::string testName = ::testing::UnitTest::GetInstance()->current_test_info()->name();
    std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
        std::make_shared<Common::CurlWrapper::CurlWrapper>();
    Common::HttpRequestsImpl::HttpRequesterImpl client = Common::HttpRequestsImpl::HttpRequesterImpl(curlWrapper);
    std::string url = URL_WITH_PORT + "/" + testName;
    Common::HttpRequests::Response response = client.get(Common::HttpRequests::RequestConfig{
        .url = url, .parameters = Common::HttpRequests::Parameters{ { "param1", "value1" } } });

    ASSERT_EQ(response.status, 200);
    std::string expected_response = "/" + testName + " response body";
    ASSERT_EQ(response.body, expected_response);
}

TEST_F(HttpRequesterLiveNetworkTests, getWithFileDownloadAndExplicitFileName)
{
    std::string testName = ::testing::UnitTest::GetInstance()->current_test_info()->name();
    std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
        std::make_shared<Common::CurlWrapper::CurlWrapper>();
    Common::HttpRequestsImpl::HttpRequesterImpl client = Common::HttpRequestsImpl::HttpRequesterImpl(curlWrapper);
    std::string url = URL_WITH_PORT + "/" + testName;

    std::string fileDownloadLocation = "/tmp/" + testName + ".txt";
    m_filesToRemove.push_back(fileDownloadLocation);
    std::string fileExpectedContent = "Sample text file to use in HTTP tests.";

    Common::HttpRequests::Response response = client.get(Common::HttpRequests::RequestConfig{
        .url = url,
        .fileDownloadLocation = fileDownloadLocation});

    auto fs = Common::FileSystem::fileSystem();
    ASSERT_TRUE(fs->exists(fileDownloadLocation));
    ASSERT_EQ(fs->readFile(fileDownloadLocation), fileExpectedContent);
    ASSERT_EQ(response.status, 200);

    // Downloaded to file so body will be empty
    ASSERT_EQ(response.body, "");

}

TEST_F(HttpRequesterLiveNetworkTests, getWithFileDownloadToDirectory)
{
    std::string testName = ::testing::UnitTest::GetInstance()->current_test_info()->name();
    std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
        std::make_shared<Common::CurlWrapper::CurlWrapper>();
    Common::HttpRequestsImpl::HttpRequesterImpl client = Common::HttpRequestsImpl::HttpRequesterImpl(curlWrapper);
    std::string url = URL_WITH_PORT + "/" + testName + "/sample.txt";

    std::string fileDownloadLocationDir = "/tmp";
    std::string fileDownloadLocationFullPath = fileDownloadLocationDir + "/sample.txt";
    m_filesToRemove.push_back(fileDownloadLocationFullPath);
    std::string fileExpectedContent = "Sample text file to use in HTTP tests.";

    Common::HttpRequests::Response response = client.get(Common::HttpRequests::RequestConfig{
        .url = url,
        .fileDownloadLocation = fileDownloadLocationDir});

    auto fs = Common::FileSystem::fileSystem();
    ASSERT_TRUE(fs->exists(fileDownloadLocationFullPath));
    ASSERT_EQ(fs->readFile(fileDownloadLocationFullPath), fileExpectedContent);
    ASSERT_EQ(response.status, 200);

    // Downloaded to file so body will be empty
    ASSERT_EQ(response.body, "");
}

TEST_F(HttpRequesterLiveNetworkTests, getWithFileDownloadToDirectoryAndContentDispositionHeader)
{
    std::string testName = ::testing::UnitTest::GetInstance()->current_test_info()->name();
    std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
        std::make_shared<Common::CurlWrapper::CurlWrapper>();
    Common::HttpRequestsImpl::HttpRequesterImpl client = Common::HttpRequestsImpl::HttpRequesterImpl(curlWrapper);
    std::string url = URL_WITH_PORT + "/" + testName + "/sample.txt";

    std::string fileDownloadLocationDir = "/tmp";
    std::string fileDownloadLocationFullPath = fileDownloadLocationDir + "/differentName.txt";
    m_filesToRemove.push_back(fileDownloadLocationFullPath);
    std::string fileExpectedContent = "Sample text file to use in HTTP tests.";

    Common::HttpRequests::Response response = client.get(Common::HttpRequests::RequestConfig{
        .url = url,
        .fileDownloadLocation = fileDownloadLocationDir});

    auto fs = Common::FileSystem::fileSystem();
    ASSERT_TRUE(fs->exists(fileDownloadLocationFullPath));
    ASSERT_EQ(fs->readFile(fileDownloadLocationFullPath), fileExpectedContent);
    ASSERT_EQ(response.status, 200);

    // Downloaded to file so body will be empty
    ASSERT_EQ(response.body, "");
}

TEST_F(HttpRequesterLiveNetworkTests, getWithPortAndTimeout)
{
    std::string testName = ::testing::UnitTest::GetInstance()->current_test_info()->name();
    std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
        std::make_shared<Common::CurlWrapper::CurlWrapper>();
    Common::HttpRequestsImpl::HttpRequesterImpl client = Common::HttpRequestsImpl::HttpRequesterImpl(curlWrapper);
    std::string url = URL_WITH_PORT + "/" + testName;
    Common::HttpRequests::Response response = client.get(Common::HttpRequests::RequestConfig{
        .url = url,
        .timeout = 3
    });

    ASSERT_EQ(response.status, Common::HttpRequests::STATUS_NOT_SET);
    ASSERT_EQ(response.body, "");
    ASSERT_EQ(response.errorCode, Common::HttpRequests::ResponseErrorCode::TIMEOUT);
    ASSERT_EQ(response.error, "Timeout was reached");
}

TEST_F(HttpRequesterLiveNetworkTests, getWithPortAndBandwidthLimit)
{
    std::string testName = ::testing::UnitTest::GetInstance()->current_test_info()->name();
    std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
        std::make_shared<Common::CurlWrapper::CurlWrapper>();
    Common::HttpRequestsImpl::HttpRequesterImpl client = Common::HttpRequestsImpl::HttpRequesterImpl(curlWrapper);
    std::string url = URL_WITH_PORT + "/" + testName;

    auto start = std::chrono::high_resolution_clock::now();

    // Download a 1000B file at ~100B/s per second
    Common::HttpRequests::Response response = client.get(Common::HttpRequests::RequestConfig{
        .url = url,
        .bandwidthLimit = 100
    });

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

TEST_F(HttpRequesterLiveNetworkTests, putWithPort)
{
    std::string testName = ::testing::UnitTest::GetInstance()->current_test_info()->name();
    std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
        std::make_shared<Common::CurlWrapper::CurlWrapper>();
    Common::HttpRequestsImpl::HttpRequesterImpl client = Common::HttpRequestsImpl::HttpRequesterImpl(curlWrapper);
    std::string url = URL_WITH_PORT + "/" + testName;
    Common::HttpRequests::Response response = client.put(Common::HttpRequests::RequestConfig{ .url = url });

    ASSERT_EQ(response.status, 200);
    std::string expected_response = "/" + testName + " response body";
    ASSERT_EQ(response.body, expected_response);
    ASSERT_EQ(response.headers.count("test_header"), 1);
    ASSERT_EQ(response.headers.at("test_header"), "test_header_value");
}

TEST_F(HttpRequesterLiveNetworkTests, putWithFileUpload)
{
    m_filesToRemove.emplace_back("/tmp/testPutFile");
    system("bash -c 'echo test > /tmp/testPutFile'");
    // "test" is 4 bytes.
    std::string fileSize = "4";

    std::string testName = ::testing::UnitTest::GetInstance()->current_test_info()->name();
    std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
        std::make_shared<Common::CurlWrapper::CurlWrapper>();
    Common::HttpRequestsImpl::HttpRequesterImpl client = Common::HttpRequestsImpl::HttpRequesterImpl(curlWrapper);
    std::string url = URL_WITH_PORT + "/" + testName;
    Common::HttpRequests::Response response = client.put(Common::HttpRequests::RequestConfig{
        .url = url,
        .headers = Common::HttpRequests::Headers{ { "Content-Length", "4"} },
        .fileToUpload = "/tmp/testPutFile"
    });

    ASSERT_EQ(response.status, 200);
    std::string expected_response = "/" + testName + " response body, you PUT 4 bytes";
    ASSERT_EQ(response.body, expected_response);
    ASSERT_EQ(response.headers.count("test_header"), 1);
    ASSERT_EQ(response.headers.at("test_header"), "test_header_value");
}

// POST Tests

TEST_F(HttpRequesterLiveNetworkTests, postWithPort)
{
    std::string testName = ::testing::UnitTest::GetInstance()->current_test_info()->name();
    std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
        std::make_shared<Common::CurlWrapper::CurlWrapper>();
    Common::HttpRequestsImpl::HttpRequesterImpl client = Common::HttpRequestsImpl::HttpRequesterImpl(curlWrapper);
    std::string url = URL_WITH_PORT + "/" + testName;
    Common::HttpRequests::Response response = client.post(Common::HttpRequests::RequestConfig{ .url = url });

    ASSERT_EQ(response.status, 200);
    std::string expected_response = "/" + testName + " response body";
    ASSERT_EQ(response.body, expected_response);
}

TEST_F(HttpRequesterLiveNetworkTests, postWithData)
{
    std::string testName = ::testing::UnitTest::GetInstance()->current_test_info()->name();
    std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
        std::make_shared<Common::CurlWrapper::CurlWrapper>();
    Common::HttpRequestsImpl::HttpRequesterImpl client = Common::HttpRequestsImpl::HttpRequesterImpl(curlWrapper);
    std::string url = URL_WITH_PORT + "/" + testName;
    Common::HttpRequests::Response response = client.post(Common::HttpRequests::RequestConfig{
        .url = url,
        .data = "SamplePostData"
    });

    ASSERT_EQ(response.status, 200);
    std::string expected_response = "/" + testName + " response body";
    ASSERT_EQ(response.body, expected_response);
}
