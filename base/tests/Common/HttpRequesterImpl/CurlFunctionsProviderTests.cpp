
#include "../Helpers/LogInitializedTests.h"
#include "FileSystem/IFileSystem.h"
#include "HttpRequestsImpl/CurlFunctionsProvider.h"
#include "../Helpers/MockFileSystem.h"
#include "../Helpers/FileSystemReplaceAndRestore.h"
#include <Common/HttpRequestsImpl/HttpRequesterImpl.h>
#include <curl/curl.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

class CurlFunctionsProviderTests : public LogInitializedTests
{};

// curlWriteFunc Tests

TEST_F(CurlFunctionsProviderTests, curlWriteFunc)
{
    const char* dataChunk1 = "This is some ";
    const char* dataChunk2 = "data to deal with";
    size_t length1 = strlen(dataChunk1);
    size_t length2 = strlen(dataChunk2);

    std::string buffer;
    size_t bytesDealtWith = 0;
    bytesDealtWith += CurlFunctionsProvider::curlWriteFunc((void*)dataChunk1, 1, length1, &buffer);
    bytesDealtWith += CurlFunctionsProvider::curlWriteFunc((void*)dataChunk2, 1, length2, &buffer);

    ASSERT_EQ(bytesDealtWith, length1 + length2);
    ASSERT_EQ(buffer, "This is some data to deal with");
}

TEST_F(CurlFunctionsProviderTests, curlWriteFuncZeroSize)
{
    const char* dataChunk1 = "";
    std::string buffer;
    size_t bytesDealtWith = 0;
    bytesDealtWith += CurlFunctionsProvider::curlWriteFunc((void*)dataChunk1, 1, 0, &buffer);
    ASSERT_EQ(bytesDealtWith, 0);
    ASSERT_EQ(buffer, "");
}


// curlWriteFileFunc Tests

TEST_F(CurlFunctionsProviderTests, curlWriteFileFuncWithNamedFile)
{
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });

    const char* dataChunk1 = "This is some ";
    const char* dataChunk2 = "data to deal with";
    size_t length1 = strlen(dataChunk1);
    size_t length2 = strlen(dataChunk2);

    Common::HttpRequestsImpl::ResponseBuffer buffer;
    buffer.downloadFilePath = "/save/to/this/file";
    buffer.url = "https://example.com";

    EXPECT_CALL(*mockFileSystem, appendFile(buffer.downloadFilePath, std::string(dataChunk1)));
    EXPECT_CALL(*mockFileSystem, appendFile(buffer.downloadFilePath, std::string(dataChunk2)));

    size_t bytesDealtWith = 0;
    bytesDealtWith += CurlFunctionsProvider::curlWriteFileFunc((void*)dataChunk1, 1, length1, &buffer);
    bytesDealtWith += CurlFunctionsProvider::curlWriteFileFunc((void*)dataChunk2, 1, length2, &buffer);

    ASSERT_EQ(bytesDealtWith, length1 + length2);
}

TEST_F(CurlFunctionsProviderTests, curlWriteFileFuncWithNamedDirectory)
{
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });

    const char* dataChunk1 = "This is some ";
    const char* dataChunk2 = "data to deal with";
    size_t length1 = strlen(dataChunk1);
    size_t length2 = strlen(dataChunk2);

    Common::HttpRequestsImpl::ResponseBuffer buffer;
    buffer.url = "https://example.com/file.txt";
    buffer.downloadDirectory = "/save/to/this/dir";
    std::string expectedDownloadLocation = Common::FileSystem::join(buffer.downloadDirectory, "file.txt");

    EXPECT_CALL(*mockFileSystem, appendFile(expectedDownloadLocation, std::string(dataChunk1)));
    EXPECT_CALL(*mockFileSystem, appendFile(expectedDownloadLocation, std::string(dataChunk2)));

    size_t bytesDealtWith = 0;
    bytesDealtWith += CurlFunctionsProvider::curlWriteFileFunc((void*)dataChunk1, 1, length1, &buffer);
    bytesDealtWith += CurlFunctionsProvider::curlWriteFileFunc((void*)dataChunk2, 1, length2, &buffer);

    ASSERT_EQ(bytesDealtWith, length1 + length2);
}

TEST_F(CurlFunctionsProviderTests, curlWriteFileFuncWithNamedDirectoryAndSlash)
{
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });

    const char* dataChunk1 = "This is some ";
    const char* dataChunk2 = "data to deal with";
    size_t length1 = strlen(dataChunk1);
    size_t length2 = strlen(dataChunk2);

    Common::HttpRequestsImpl::ResponseBuffer buffer;
    buffer.url = "https://example.com/file.txt";
    buffer.downloadDirectory = "/save/to/this/dir/"; // <-- slash here
    std::string expectedDownloadLocation = Common::FileSystem::join(buffer.downloadDirectory, "file.txt");

    EXPECT_CALL(*mockFileSystem, appendFile(expectedDownloadLocation, std::string(dataChunk1)));
    EXPECT_CALL(*mockFileSystem, appendFile(expectedDownloadLocation, std::string(dataChunk2)));

    size_t bytesDealtWith = 0;
    bytesDealtWith += CurlFunctionsProvider::curlWriteFileFunc((void*)dataChunk1, 1, length1, &buffer);
    bytesDealtWith += CurlFunctionsProvider::curlWriteFileFunc((void*)dataChunk2, 1, length2, &buffer);

    ASSERT_EQ(bytesDealtWith, length1 + length2);
}

TEST_F(CurlFunctionsProviderTests, curlWriteFileFuncWithNoTargetFileOrDirShouldReturnZero)
{
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });

    const char* dataChunk1 = "This is some ";
    const char* dataChunk2 = "data to deal with";
    size_t length1 = strlen(dataChunk1);
    size_t length2 = strlen(dataChunk2);

    Common::HttpRequestsImpl::ResponseBuffer buffer;
    buffer.url = "https://example.com";

    size_t bytesDealtWith = 0;
    bytesDealtWith += CurlFunctionsProvider::curlWriteFileFunc((void*)dataChunk1, 1, length1, &buffer);
    bytesDealtWith += CurlFunctionsProvider::curlWriteFileFunc((void*)dataChunk2, 1, length2, &buffer);

    ASSERT_EQ(bytesDealtWith, 0);
}

TEST_F(CurlFunctionsProviderTests, curlWriteFileFuncWithDirAndContentDispositionHeader)
{
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });

    const char* dataChunk1 = "This is some ";
    const char* dataChunk2 = "data to deal with";
    size_t length1 = strlen(dataChunk1);
    size_t length2 = strlen(dataChunk2);

    Common::HttpRequestsImpl::ResponseBuffer buffer;
    buffer.url = "https://example.com";
    buffer.downloadDirectory = "/this/dir/";
    buffer.headers = Common::HttpRequests::Headers {{"Content-Disposition","apple=tree; filename=downloadme.something.zip; another=tree"}};
    std::string expectedDownloadLocation = Common::FileSystem::join( buffer.downloadDirectory, "downloadme.something.zip");

    EXPECT_CALL(*mockFileSystem, appendFile(expectedDownloadLocation, std::string(dataChunk1)));
    EXPECT_CALL(*mockFileSystem, appendFile(expectedDownloadLocation, std::string(dataChunk2)));

    size_t bytesDealtWith = 0;
    bytesDealtWith += CurlFunctionsProvider::curlWriteFileFunc((void*)dataChunk1, 1, length1, &buffer);
    bytesDealtWith += CurlFunctionsProvider::curlWriteFileFunc((void*)dataChunk2, 1, length2, &buffer);

    ASSERT_EQ(bytesDealtWith, length1 + length2);
}


// curlWriteDebugFunc Tests

TEST_F(CurlFunctionsProviderTests, curlWriteDebugFuncText)
{
    testing::internal::CaptureStderr();
    void* fakeCurlHandle = reinterpret_cast<void*>(0xdeadbeef); // unused
    void* fakeUserPtr = reinterpret_cast<void*>(0xddddaaaa); // unused
    char someDebugData[] = "This is some debug data";
    char* dataPtr = someDebugData;
    auto rc =  CurlFunctionsProvider::curlWriteDebugFunc(fakeCurlHandle, curl_infotype::CURLINFO_TEXT, dataPtr, strlen(dataPtr), fakeUserPtr);
    std::string logMessages = testing::internal::GetCapturedStderr();
    std::string expected = "cURL Info: " + std::string (someDebugData);
    EXPECT_THAT(logMessages, ::testing::HasSubstr(expected));
    ASSERT_EQ(rc, 0);
}

TEST_F(CurlFunctionsProviderTests, curlWriteDebugFuncDataIn)
{
    testing::internal::CaptureStderr();
    void* fakeCurlHandle = reinterpret_cast<void*>(0xdeadbeef); // unused
    void* fakeUserPtr = reinterpret_cast<void*>(0xddddaaaa); // unused
    char someDebugData[] = "This is some debug data";
    char* dataPtr = someDebugData;
    auto rc =  CurlFunctionsProvider::curlWriteDebugFunc(fakeCurlHandle, curl_infotype::CURLINFO_DATA_IN, dataPtr, strlen(dataPtr), fakeUserPtr);
    std::string logMessages = testing::internal::GetCapturedStderr();
    std::string expected = "cURL <= Recv data: " + std::string (someDebugData);
    EXPECT_THAT(logMessages, ::testing::HasSubstr(expected));
    ASSERT_EQ(rc, 0);
}

TEST_F(CurlFunctionsProviderTests, curlWriteDebugFuncDataOut)
{
    testing::internal::CaptureStderr();
    void* fakeCurlHandle = reinterpret_cast<void*>(0xdeadbeef); // unused
    void* fakeUserPtr = reinterpret_cast<void*>(0xddddaaaa); // unused
    char someDebugData[] = "This is some debug data";
    char* dataPtr = someDebugData;
    auto rc =  CurlFunctionsProvider::curlWriteDebugFunc(fakeCurlHandle, curl_infotype::CURLINFO_DATA_OUT, dataPtr, strlen(dataPtr), fakeUserPtr);
    std::string logMessages = testing::internal::GetCapturedStderr();
    std::string expected = "cURL => Send data: " + std::string (someDebugData);
    EXPECT_THAT(logMessages, ::testing::HasSubstr(expected));
    ASSERT_EQ(rc, 0);
}

TEST_F(CurlFunctionsProviderTests, curlWriteDebugFuncHeaderIn)
{
    testing::internal::CaptureStderr();
    void* fakeCurlHandle = reinterpret_cast<void*>(0xdeadbeef); // unused
    void* fakeUserPtr = reinterpret_cast<void*>(0xddddaaaa); // unused
    char someDebugData[] = "This is some debug data";
    char* dataPtr = someDebugData;
    auto rc =  CurlFunctionsProvider::curlWriteDebugFunc(fakeCurlHandle, curl_infotype::CURLINFO_HEADER_IN, dataPtr, strlen(dataPtr), fakeUserPtr);
    std::string logMessages = testing::internal::GetCapturedStderr();
    std::string expected = "cURL <= Recv header: " + std::string (someDebugData);
    EXPECT_THAT(logMessages, ::testing::HasSubstr(expected));
    ASSERT_EQ(rc, 0);
}

TEST_F(CurlFunctionsProviderTests, curlWriteDebugFuncHeaderOut)
{
    testing::internal::CaptureStderr();
    void* fakeCurlHandle = reinterpret_cast<void*>(0xdeadbeef); // unused
    void* fakeUserPtr = reinterpret_cast<void*>(0xddddaaaa); // unused
    char someDebugData[] = "This is some debug data";
    char* dataPtr = someDebugData;
    auto rc =  CurlFunctionsProvider::curlWriteDebugFunc(fakeCurlHandle, curl_infotype::CURLINFO_HEADER_OUT, dataPtr, strlen(dataPtr), fakeUserPtr);
    std::string logMessages = testing::internal::GetCapturedStderr();
    std::string expected = "cURL => Send header: " + std::string (someDebugData);
    EXPECT_THAT(logMessages, ::testing::HasSubstr(expected));
    ASSERT_EQ(rc, 0);
}

TEST_F(CurlFunctionsProviderTests, curlWriteDebugFuncSslDataIn)
{
    testing::internal::CaptureStderr();
    void* fakeCurlHandle = reinterpret_cast<void*>(0xdeadbeef); // unused
    void* fakeUserPtr = reinterpret_cast<void*>(0xddddaaaa); // unused
    char someDebugData[] = "This is some debug data";
    char* dataPtr = someDebugData;
    auto rc =  CurlFunctionsProvider::curlWriteDebugFunc(fakeCurlHandle, curl_infotype::CURLINFO_SSL_DATA_IN, dataPtr, strlen(dataPtr), fakeUserPtr);
    std::string logMessages = testing::internal::GetCapturedStderr();
    std::string expected = "cURL <= Recv SSL data: " + std::string (someDebugData);
    EXPECT_THAT(logMessages, ::testing::HasSubstr(expected));
    ASSERT_EQ(rc, 0);
}

TEST_F(CurlFunctionsProviderTests, curlWriteDebugFuncSslDataOut)
{
    testing::internal::CaptureStderr();
    void* fakeCurlHandle = reinterpret_cast<void*>(0xdeadbeef); // unused
    void* fakeUserPtr = reinterpret_cast<void*>(0xddddaaaa); // unused
    char someDebugData[] = "This is some debug data";
    char* dataPtr = someDebugData;
    auto rc =  CurlFunctionsProvider::curlWriteDebugFunc(fakeCurlHandle, curl_infotype::CURLINFO_SSL_DATA_OUT, dataPtr, strlen(dataPtr), fakeUserPtr);
    std::string logMessages = testing::internal::GetCapturedStderr();
    std::string expected = "cURL => Send SSL data: " + std::string (someDebugData);
    EXPECT_THAT(logMessages, ::testing::HasSubstr(expected));
    ASSERT_EQ(rc, 0);
}