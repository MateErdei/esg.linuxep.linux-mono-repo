// Copyright 2023 Sophos All rights reserved.
#pragma once

#include "modules/Common/Exceptions/IException.h"

namespace Common::Policy
{
    class PolicyParseException : public Common::Exceptions::IException
    {
    public:
        using Common::Exceptions::IException::IException;
    };
}
