// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Exceptions/IException.h"

namespace Common::PluginCommunication
{
    class IPluginCommunicationException : public Exceptions::IException
    {
    public:
        using Exceptions::IException::IException;
    };
} // namespace Common::PluginCommunication
