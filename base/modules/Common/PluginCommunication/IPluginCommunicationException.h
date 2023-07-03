// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "modules/Common/Exceptions/IException.h"

namespace Common
{
    namespace PluginCommunication
    {
        class IPluginCommunicationException : public Exceptions::IException
        {
        public:
            using Exceptions::IException::IException;
        };
    } // namespace PluginCommunication
} // namespace Common
