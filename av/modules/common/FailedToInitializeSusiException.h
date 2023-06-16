// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "datatypes/AVException.h"

class FailedToInitializeSusiException : public datatypes::AVException
{
public:
    using datatypes::AVException::AVException;
};
