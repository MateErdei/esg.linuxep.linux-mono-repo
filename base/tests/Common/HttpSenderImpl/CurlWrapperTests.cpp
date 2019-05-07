/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

/**
 * Component tests for libcurl wrapper
 */

#include <Common/HttpSenderImpl/CurlWrapper.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>

class FakeCurlWrapper : public Common::HttpSenderImpl::CurlWrapper
{
public:
    CURLcode curlEasyPerform(CURL*) override
    {
        return CURLE_OK;
    }
};

class CurlWrapperTest : public ::testing::Test
{
public:
    long m_flags = 0;
    std::shared_ptr<FakeCurlWrapper> m_curlWrapper;

    void SetUp() override
    {
        m_curlWrapper = std::make_shared<FakeCurlWrapper>();
    }
};

TEST_F(CurlWrapperTest, curlGlobalInit) // NOLINT
{
    EXPECT_EQ(m_curlWrapper->curlGlobalInit(m_flags), CURLE_OK);
    EXPECT_NO_THROW(m_curlWrapper->curlGlobalCleanup()); // NOLINT
}

TEST_F(CurlWrapperTest, curlEasyInit) // NOLINT
{
    m_curlWrapper->curlGlobalInit(m_flags);
    CURL* handle = m_curlWrapper->curlEasyInit();
    EXPECT_NE(handle, nullptr);
    EXPECT_NO_THROW(m_curlWrapper->curlEasyCleanup(handle)); // NOLINT
    EXPECT_NO_THROW(m_curlWrapper->curlGlobalCleanup()); // NOLINT
}

TEST_F(CurlWrapperTest, curlEasySetOpt) // NOLINT
{
    m_curlWrapper->curlGlobalInit(m_flags);
    CURL* handle = m_curlWrapper->curlEasyInit();
    EXPECT_EQ(m_curlWrapper->curlEasySetOpt(handle, CURLOPT_URL, "https://localhost"), CURLE_OK);
    EXPECT_NO_THROW(m_curlWrapper->curlEasyCleanup(handle)); // NOLINT
    EXPECT_NO_THROW(m_curlWrapper->curlGlobalCleanup()); // NOLINT
}

TEST_F(CurlWrapperTest, curlSlistAppend) // NOLINT
{
    m_curlWrapper->curlGlobalInit(m_flags);
    curl_slist* slist = nullptr;
    slist = m_curlWrapper->curlSlistAppend(slist, "slist_value");
    EXPECT_NE(slist, nullptr);
    EXPECT_NO_THROW(m_curlWrapper->curlSlistFreeAll(slist)); // NOLINT
    EXPECT_NO_THROW(m_curlWrapper->curlGlobalCleanup()); // NOLINT
}

TEST_F(CurlWrapperTest, curlEasyPerform) // NOLINT
{
    m_curlWrapper->curlGlobalInit(m_flags);
    CURL* handle = m_curlWrapper->curlEasyInit();
    EXPECT_EQ(m_curlWrapper->curlEasyPerform(handle), CURLE_OK);
    EXPECT_NO_THROW(m_curlWrapper->curlEasyCleanup(handle)); // NOLINT
    EXPECT_NO_THROW(m_curlWrapper->curlGlobalCleanup()); // NOLINT
}

TEST_F(CurlWrapperTest, curlEasyStrError) // NOLINT
{
    m_curlWrapper->curlGlobalInit(m_flags);
    EXPECT_NE(m_curlWrapper->curlEasyStrError(CURLE_OK), "");
    EXPECT_NO_THROW(m_curlWrapper->curlGlobalCleanup()); // NOLINT
}