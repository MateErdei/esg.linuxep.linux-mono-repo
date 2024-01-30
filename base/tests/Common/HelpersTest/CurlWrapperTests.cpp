// Copyright 2019-2023 Sophos Limited. All rights reserved.

/**
 * Component tests for libcurl wrapper
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include "Common/CurlWrapper/CurlWrapper.h"

class FakeCurlWrapper : public Common::CurlWrapper::CurlWrapper
{
public:
    CURLcode curlEasyPerform(CURL*) override { return CURLE_OK; }
};

class CurlWrapperTest : public ::testing::Test
{
public:
    long m_flags = 0;
    std::shared_ptr<FakeCurlWrapper> m_curlWrapper;

    void SetUp() override { m_curlWrapper = std::make_shared<FakeCurlWrapper>(); }
};

TEST_F(CurlWrapperTest, curlGlobalInit)
{
    EXPECT_EQ(m_curlWrapper->curlGlobalInit(m_flags), CURLE_OK);
    EXPECT_NO_THROW(m_curlWrapper->curlGlobalCleanup());
}

TEST_F(CurlWrapperTest, curlEasyInit)
{
    m_curlWrapper->curlGlobalInit(m_flags);
    CURL* handle = m_curlWrapper->curlEasyInit();
    EXPECT_NE(handle, nullptr);
    EXPECT_NO_THROW(m_curlWrapper->curlEasyCleanup(handle));
    EXPECT_NO_THROW(m_curlWrapper->curlGlobalCleanup());
}

TEST_F(CurlWrapperTest, curlEasySetOpt)
{
    m_curlWrapper->curlGlobalInit(m_flags);
    CURL* handle = m_curlWrapper->curlEasyInit();
    EXPECT_EQ(m_curlWrapper->curlEasySetOpt(handle, CURLOPT_URL, "https://localhost"), CURLE_OK);
    EXPECT_NO_THROW(m_curlWrapper->curlEasyCleanup(handle));
    EXPECT_NO_THROW(m_curlWrapper->curlGlobalCleanup());
}

TEST_F(CurlWrapperTest, curlSlistAppend)
{
    m_curlWrapper->curlGlobalInit(m_flags);
    curl_slist* slist = nullptr;
    slist = m_curlWrapper->curlSlistAppend(slist, "slist_value");
    EXPECT_NE(slist, nullptr);
    EXPECT_NO_THROW(m_curlWrapper->curlSlistFreeAll(slist));
    EXPECT_NO_THROW(m_curlWrapper->curlGlobalCleanup());
}

TEST_F(CurlWrapperTest, curlEasyPerform)
{
    m_curlWrapper->curlGlobalInit(m_flags);
    CURL* handle = m_curlWrapper->curlEasyInit();
    EXPECT_EQ(m_curlWrapper->curlEasyPerform(handle), CURLE_OK);
    EXPECT_NO_THROW(m_curlWrapper->curlEasyCleanup(handle));
    EXPECT_NO_THROW(m_curlWrapper->curlGlobalCleanup());
}

TEST_F(CurlWrapperTest, curlEasyStrError)
{
    m_curlWrapper->curlGlobalInit(m_flags);
    EXPECT_NE(m_curlWrapper->curlEasyStrError(CURLE_OK), "");
    EXPECT_NO_THROW(m_curlWrapper->curlGlobalCleanup());
}