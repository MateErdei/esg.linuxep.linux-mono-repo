/******************************************************************************************************

Copyright 2018-2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>

// ------------------------------------------------------------------------------------------------
// Base64 declaration
// ------------------------------------------------------------------------------------------------
namespace Common::ObfuscationImpl
{
    class Base64
    {
    public:
        // Decode a base64 string.

        static std::string Decode(const std::string& sEncoded);
        static std::string Encode(const std::string& content);
    };
} // namespace Common::ObfuscationImpl