// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

#include "datatypes/AVException.h"

namespace common
{
    class AbortScanException : public datatypes::AVException
    {
    public:
        using datatypes::AVException::AVException;
    };
}
