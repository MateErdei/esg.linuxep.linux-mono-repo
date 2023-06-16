// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "datatypes/AVException.h"

namespace unixsocket
{
    class UnixSocketException : public datatypes::AVException
    {
    public:
        using datatypes::AVException::AVException;
    };
}
