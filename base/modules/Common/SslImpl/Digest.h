// Copyright 2018-2022, Sophos Limited. All rights reserved.

#pragma once

#include <istream>

namespace Common::SslImpl
{
    enum class Digest
    {
        md5,
        sha256,
        sha1
    };

    /**
     * Calculates the digest of data from an input stream.
     * @param digestName The type of digest to calculate.
     * @param inStream Input stream.
     * @return Lowercase hexadecimal representation of the digest.
     */
    [[nodiscard]] std::string calculateDigest(Digest digestName, std::istream& inStream);

    /**
     * Calculates the digest of a string.
     * @param digestName The type of digest to calculate.
     * @param input The input string.
     * @return Lowercase hexadecimal representation of the digest.
     */
    [[nodiscard]] std::string calculateDigest(Digest digestName, const std::string& input);

    constexpr int digestBufferSize = 1024;
} // namespace Common::SslImpl