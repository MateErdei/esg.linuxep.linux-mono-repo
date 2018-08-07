/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <Common/Datatypes/StringVector.h>

namespace wdctl
{
    namespace wdctlarguments
    {
        class Arguments
        {
        public:
            using StringVector = Common::Datatypes::StringVector;
            void parseArguments(const StringVector& args);
            StringVector m_positionalArgs;
            StringVector m_options;
            std::string m_command;
            std::string m_argument;
        };
    }
}
