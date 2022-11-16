// Copyright 2022, Sophos Limited. All rights reserved.

#include "Uuid.h"

#include "Common/SslImpl/Digest.h"

namespace Common::UtilityImpl::Uuid
{
    bool IsValid(const std::string& uuidString) noexcept
    {
        if (uuidString.size() != 36)
        {
            return false;
        }

        for (int i = 0; i < 36; ++i)
        {
            char c = uuidString[i];
            if (i == 8 || i == 13 || i == 18 || i == 23)
            {
                if (c != '-')
                {
                    return false;
                }
            }
            else if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))
            {
                return false;
            }
        }

        return true;
    }

    std::string CreateVersion5(const Bytes& ns, const std::string& name)
    {
        // Generate SHA-1 of concatenated namespace bytes and name, and truncate it to 128 bits (32 hex chars)
        std::string string =
            Common::SslImpl::calculateDigest(
                Common::SslImpl::Digest::sha1, std::string(reinterpret_cast<const char*>(ns.data()), 16) + name)
                .substr(0, 32);
        // Insert hyphens
        string.insert(20, 1, '-');
        string.insert(16, 1, '-');
        string.insert(12, 1, '-');
        string.insert(8, 1, '-');

        // Set version and variant
        // Version is 4 bits and stored under 'M'
        // Variant is top 2 bits of 'N'
        // xxxxxxxx-xxxx-Mxxx-Nxxx-xxxxxxxxxxxx
        string[14] = '5';                                            // Version 5
        string[19] = NibbleToHex(8 | (HexToNibble(string[19]) & 3)); // Variant 1 (2 bits) + 2 hash bits

        return string;
    }
} // namespace Common::UtilityImpl::Uuid