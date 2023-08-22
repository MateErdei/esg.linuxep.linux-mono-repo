// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Exceptions/IException.h"

namespace Common::PluginApi
{
    /**
     * Exception class to report failures when handling the api requests.
     */
    class ApiException : public Common::Exceptions::IException
    {
    public:
        using Common::Exceptions::IException::IException;
    };
} // namespace Common::PluginApi
