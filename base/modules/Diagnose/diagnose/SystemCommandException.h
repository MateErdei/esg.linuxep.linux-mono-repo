/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/Process/IProcessException.h>

namespace diagnose
{
    class SystemCommandsException : public Common::Process::IProcessException
    {
    public:
        SystemCommandsException(const std::string& what, const std::string& output) :
            IProcessException(what),
            m_message(output)
        {
        }

        std::string output() const { return m_message; }

    private:
        std::string m_message;
    };
} // namespace diagnose
