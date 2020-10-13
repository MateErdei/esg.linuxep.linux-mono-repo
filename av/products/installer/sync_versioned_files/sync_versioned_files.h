/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "datatypes/sophos_filesystem.h"

namespace fs = sophos_filesystem;

namespace sync_versioned_files
{
    int sync_versioned_files(const fs::path& src, const fs::path& dest);

    bool startswith(const fs::path& p, const fs::path& current_stem);
}