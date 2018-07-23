/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef MANAGEMENTAGENT_PLUGINCOMMUNICATION_IPLUGINCOMMUNICATIONEXCEPTION_H
#define MANAGEMENTAGENT_PLUGINCOMMUNICATION_IPLUGINCOMMUNICATIONEXCEPTION_H


#include "Common/Exceptions/IException.h"

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

#endif //MANAGEMENTAGENT_PLUGINCOMMUNICATION_IPLUGINCOMMUNICATIONEXCEPTION_H
