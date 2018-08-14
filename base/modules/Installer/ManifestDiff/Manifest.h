/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "ManifestEntry.h"

#include <vector>

namespace Installer
{
    namespace ManifestDiff
    {
        /**
         * Represent a manifest file.
         */
        class Manifest
        {
        public:
            explicit Manifest(std::istream& file);
            static Manifest ManifestFromPath(const std::string& filepath);

            /**
             * Convert a Manifest path to a posix path (And remove leading ./)
             *
             * .\install.sh to install.sh
             * @param p
             * @return
             */
            static std::string toPosixPath(const std::string& p);
        private:
            std::vector<ManifestEntry> m_entries;
        };
    }
}


