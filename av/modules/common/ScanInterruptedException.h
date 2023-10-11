// Copyright 2021-2023 Sophos Limited. All rights reserved.

#pragma once

#include "datatypes/AVException.h"

class ScanInterruptedException : public datatypes::AVException
{
public:
    using datatypes::AVException::AVException;
};
