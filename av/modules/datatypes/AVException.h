// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Exceptions/IException.h"

namespace datatypes
{
    class AVException : public Common::Exceptions::IException
    {
    public:
        using Common::Exceptions::IException::IException;
    };
}
