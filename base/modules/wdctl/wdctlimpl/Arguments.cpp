/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "Arguments.h"

using namespace wdctl::wdctlimpl;

void Arguments::parseArguments(const StringVector& args)
{
    // ignore args[0]
    StringVector positionalArgs;
    StringVector options;


    for (auto& arg : args)
    {
        if (arg.find("--") == 0)
        {
            options.emplace_back(arg);
        }
        else
        {
            positionalArgs.emplace_back(arg);
        }
    }

    m_options = std::move(options);
    m_positionalArgs = std::move(positionalArgs);

    // m_positionalArgs[0] == "wdctl"
    if (m_positionalArgs.size() > 1)
    {
        m_command = m_positionalArgs.at(1);
        if (m_positionalArgs.size() > 2)
        {
            m_argument = m_positionalArgs.at(2);
        }
    }
}
