// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "Common/SslImpl/Digest.h"
#include <gtest/gtest.h>

using namespace Common::SslImpl;

TEST(TestDigest, calculateDigestMd5)
{
    EXPECT_EQ(calculateDigest(Digest::md5, "hello world!"), "fc3ff98e8c6a0d3087d515c0473f8677");
    EXPECT_EQ(calculateDigest(Digest::md5, "this is a\nmultiline string"), "ce9107bda89e91c8f277ace9056e1322");
    EXPECT_EQ(calculateDigest(Digest::md5, ""), "d41d8cd98f00b204e9800998ecf8427e");
}

TEST(TestDigest, calculateDigestSha256)
{
    // clang-format off
    EXPECT_EQ(calculateDigest(Digest::sha256, "hello world!"), "7509e5bda0c762d2bac7f90d758b5b2263fa01ccbc542ab5e3df163be08e6ca9");
    EXPECT_EQ(calculateDigest(Digest::sha256, "this is a\nmultiline string"), "09aff576e3459195820597c9a14cbb283e864a939055dc44e06b448f2066a993");
    EXPECT_EQ(calculateDigest(Digest::sha256, ""), "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    // clang-format on
}

TEST(TestDigest, calculateDigestInputLongerThanBufferSize)
{
    // clang-format off
    EXPECT_EQ(calculateDigest(Digest::md5, std::string(digestBufferSize + 1, '.')), "506ff924fd0061830292d99415084501");
    EXPECT_EQ(calculateDigest(Digest::md5, std::string(digestBufferSize * 2, '.')), "3a9099505930995e266e175c131507ad");
    EXPECT_EQ(calculateDigest(Digest::md5, std::string(digestBufferSize * 2 + 1, '.')), "f6d1980c19ca54e6710d83faedca329f");
    // clang-format on
}

TEST(TestDigest, calculateDigestStream)
{
    std::istringstream stream{ "hello world!" };
    EXPECT_EQ(calculateDigest(Digest::md5, stream), "fc3ff98e8c6a0d3087d515c0473f8677");
}

TEST(TestDigest, calculateDigestBadStreamThrows)
{
    std::istringstream stream{ "foo" };
    stream.setstate(std::istringstream::badbit);
    EXPECT_THROW(std::ignore = calculateDigest(Digest::md5, stream), std::runtime_error);
}