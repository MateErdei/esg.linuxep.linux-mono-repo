// Copyright 2018-2023 Sophos All rights reserved.

#include "Credentials.h"

#include "PolicyParseException.h"

#include "Common/ObfuscationImpl/Obfuscate.h"

#include <utility>

using namespace Common::Policy;

Credentials::Credentials(std::string username, std::string password) :
    username_(std::move(username)), password_(std::move(password))
{
    if (!password_.empty() && username_.empty())
    {
        bool throwInvalid = true;
        try
        {
            auto deobfuscated = Common::ObfuscationImpl::SECDeobfuscate(password_);
            // if deobfuscated is empty, we have a username and password as empty, which is valid.
            throwInvalid = !deobfuscated.empty();
        }
        catch (std::exception&)
        {
        }

        if (throwInvalid)
        {
            throw PolicyParseException(LOCATION, "Invalid credentials");
        }
    }
}
