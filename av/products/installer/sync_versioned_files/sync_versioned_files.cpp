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

void sync_versioned_files::delete_removed_file(const fs::path& p)
{
    fs::path temp = p;
    while (true)
    {
        PRINT("Delete "<< temp);
        fs::remove(temp);

        fs::path last = temp;
        temp.replace_extension();
        if (last == temp)
        {
            break;
        }
    }
}

int sync_versioned_files::sync_versioned_files(const fs::path& src, const fs::path& dest, bool isVersioned)
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
        if (isVersioned)
        {
            expected_name.replace_extension();
        }
        fs::path src_name = replace_stem(expected_name, dest, src);
        if (!fs::is_regular_file(src_name))
        {
            if (isVersioned)
            {
                // delete p and all symlinks to p created by versioned copy
                delete_removed_file(p.path());
            }
            else
            {
                fs::remove(p.path());
            }
        }
    }
    return 0;
}

static bool are_identical(const path_t& src, const path_t& dest)
{
    std::ignore = src;
    std::ignore = dest;
    return true;
}

static void copyFile(const path_t& src, const path_t& dest)
{
    std::ignore = src;
    std::ignore = dest;
}

int sync_versioned_files::copy(const path_t& src, const path_t& dest)
{
    // for each file in dest, work out what we expect as a source, and see if it is present
    for (const auto& p : fs::directory_iterator(dest))
    {
        if (!fs::is_regular_file(p))
        {
            continue;
        }
        const auto& dest_name = p.path();
        const auto src_name = replace_stem(dest_name, dest, src);
        if (!fs::is_regular_file(src_name))
        {
            fs::remove(dest_name);
            continue;
        }
        // Source exists, so compare
        if (!are_identical(src_name, dest_name))
        {
            copyFile(src_name, dest_name);
        }
    }

    // Now copy any new files
    for (const auto& p : fs::directory_iterator(src))
    {
        if (!fs::is_regular_file(p))
        {
            continue;
        }
        const auto& src_name = p.path();
        const auto dest_name = replace_stem(src_name, src, dest);

        if (!fs::is_regular_file(dest_name))
        {
            copyFile(src_name, dest_name);
        }
    }
    return 0;
}
