/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <Common/Datatypes/StringVector.h>

namespace Installer
{
    namespace ManifestDiff
    {
        class CommandLineOptions
        {
        public:
            explicit CommandLineOptions(const Common::Datatypes::StringVector& args);
            std::string m_old;
            std::string m_new;
            std::string m_changed;
            std::string m_added;
            std::string m_removed;
        };
    } // namespace ManifestDiff
} // namespace Installer
