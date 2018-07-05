//
// Created by pair on 05/07/18.
//

#ifndef EVEREST_BASE_IMANAGEMENTAGENTEXCEPTION_H
#define EVEREST_BASE_IMANAGEMENTAGENTEXCEPTION_H


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

#endif //EVEREST_BASE_IMANAGEMENTAGENTEXCEPTION_H
