/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "datatypes/sophos_filesystem.h"

namespace fs = sophos_filesystem;

namespace sync_versioned_files
{
    using path_t = fs::path;

    int copy(const path_t& src, const path_t& dest);

    int sync_versioned_files(const fs::path& src, const fs::path& dest, bool isVersioned=true);

    void delete_removed_file(const fs::path& p);

    bool startswith(const fs::path& p, const fs::path& current_stem);

    fs::path suffix(const fs::path& p, const fs::path& current_stem);

    fs::path replace_stem(const fs::path& p, const fs::path& current_stem, const fs::path& required_stem);
}