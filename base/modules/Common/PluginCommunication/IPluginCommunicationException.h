/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/Exceptions/IException.h>

namespace Common
{
    namespace PluginCommunication
    {
        class IPluginCommunicationException : public Exceptions::IException
        {
        public:
            explicit IPluginCommunicationException(const std::string& what) : Exceptions::IException(what) {}
        };
    } // namespace PluginCommunication
} // namespace Common
