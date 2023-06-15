// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "Common/Exceptions/IException.h"

class FailedToInitializeSusiException : public Common::Exceptions::IException
{
public:
    using Common::Exceptions::IException::IException;
};
