// Copyright 2018-2022, Sophos Limited. All rights reserved.

#pragma once

#include <istream>

namespace Common::SslImpl
{
    namespace Digest
    {
        constexpr const char* md5 = "md5";
        constexpr const char* sha256 = "sha256";
    } // namespace Digest

    /**
     * Calculates the digest of data supplied by an istream
     * @param digestName The name of the digest to calculate, e.g. "sha256". It is preferred to use the string variables
     * in the Common::SslImpl::Digest namespace for consistency
     * @param inStream Input stream containing data to calculate the digest of
     * @return Hexadecimal representation of the digest
     */
    [[nodiscard]] std::string calculateDigest(const char* digestName, std::istream& inStream);

    /**
     * Calculates the digest of a string
     * @param digestName The name of the digest to calculate, e.g. "sha256". It is preferred to use the string variables
     * in the Common::SslImpl::Digest namespace for consistency
     * @param inStream String to calculate the digest of
     * @return Hexadecimal representation of the digest
     */
    [[nodiscard]] std::string calculateDigest(const char* digestName, const std::string& input);

    constexpr int digestBufferSize = 1024;
} // namespace Common::SslImpl