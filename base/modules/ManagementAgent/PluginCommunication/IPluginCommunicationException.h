/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include <Common/Exceptions/IException.h>

namespace ManagementAgent
{
namespace PluginCommunication
{
    class IPluginCommunicationException : public Common::Exceptions::IException
    {
    public:
        explicit IPluginCommunicationException(const std::string &what)
                : Common::Exceptions::IException(what)
        {}
    };
}
}


