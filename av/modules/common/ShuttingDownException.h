// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "datatypes/AVException.h"

class ShuttingDownException : public datatypes::AVException
{
public:
    using datatypes::AVException::AVException;
    ShuttingDownException()
        : datatypes::AVException("")
    {}

};
