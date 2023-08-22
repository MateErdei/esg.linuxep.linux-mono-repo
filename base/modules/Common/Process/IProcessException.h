// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Exceptions/IException.h"

namespace Common::Process
{
    class IProcessException : public Common::Exceptions::IException
    {
    public:
        explicit IProcessException(const std::string& what) : Common::Exceptions::IException(what) {}
    };
} // namespace Common::Process
