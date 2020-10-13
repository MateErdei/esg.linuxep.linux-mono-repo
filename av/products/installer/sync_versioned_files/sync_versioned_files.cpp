/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "sync_versioned_files.h"

#include "datatypes/Print.h"

#include <cassert>
using namespace sync_versioned_files;

bool sync_versioned_files::startswith(const fs::path& p, const fs::path& current_stem)
{
    return p.string().find(current_stem.string()) == 0;
}

fs::path sync_versioned_files::suffix(const fs::path& p, const fs::path& current_stem)
{
    assert(startswith(p, current_stem));
    assert(p.string().length() > current_stem.string().length() + 1);
    return p.string().substr(current_stem.string().length() + 1);
}

fs::path sync_versioned_files::replace_stem(const fs::path& p, const fs::path& current_stem, const fs::path& required_stem)
{
    fs::path result = suffix(p, current_stem);

    return required_stem / result;
}

static void delete_removed_file(const fs::path& p)
{
    fs::path temp = p;
    while (true)
    {
        PRINT("Delete "<< temp);
        fs::remove(temp);
        fs::path temp2 = temp.replace_extension();
        if (temp2 == temp)
        {
            break;
        }
    }
}

int sync_versioned_files::sync_versioned_files(const fs::path& src, const fs::path& dest)
{
    // for each file in dest, work out what we expect as a source, and see if it is present
    for (const auto& p : fs::directory_iterator(dest))
    {
        if (fs::is_symlink(p))
        {
            continue;
        }
        if (!fs::is_regular_file(p))
        {
            continue;
        }
        fs::path expected_name = p.path();
        expected_name.replace_extension();
        fs::path src_name = replace_stem(expected_name, dest, src);
        if (!fs::is_regular_file(src_name))
        {
            // delete p and all symlinks to p created by versioned copy
            delete_removed_file(p.path());
        }
    }
    return 0;
}
