/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "StringVector.h"

namespace wdctl
{
    namespace wdctlimpl
    {
        class Arguments
        {
        public:
            void parseArguments(const StringVector& args);
            StringVector m_positionalArgs;
            StringVector m_options;
            std::string m_command;
            std::string m_argument;
        };
    }
}


