/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <Common/Datatypes/StringVector.h>

namespace Installer
{
    namespace ManifestDiff
    {
        class ManifestDiff
        {
        public:
            static int manifestDiffMain(int argc, char* argv[]);
            static int manifestDiffMain(const Common::Datatypes::StringVector& argv);
        };
    }
}
