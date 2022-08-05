// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#pragma once

#include "datatypes/sophos_filesystem.h"

namespace fs = sophos_filesystem;

namespace sync_versioned_files
{
    using path_t = fs::path;

    /**
     * Full synchronize between src to dest
     *
     * Copy changed files
     * Delete removed files
     * Copy new files
     *
     * @param src
     * @param dest
     * @return 0 for success, 1 for failure to copy changed files, 2 for failure to copy new files
     */
    int copy(const path_t& src, const path_t& dest);

    int sync_versioned_files(const fs::path& src, const fs::path& dest, bool isVersioned=true);

    void delete_removed_file(const fs::path& p);

    bool startswith(const fs::path& p, const fs::path& current_stem);

    fs::path suffix(const fs::path& p, const fs::path& current_stem);

    fs::path replace_stem(const fs::path& p, const fs::path& current_stem, const fs::path& required_stem);
}