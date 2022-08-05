// Copyright 2020-2022, Sophos Limited.  All rights reserved.

// Class
#include "sync_versioned_files.h"
// Component
#include "datatypes/Print.h"
// Std C++
#include <cassert>
#include <cstring>
// Std C
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

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

namespace
{
    class AutoFd
    {
    public:
        int m_fd;
        explicit AutoFd(int fd)
            : m_fd(fd)
        {}
        ~AutoFd()
        {
            if (m_fd >= 0)
            {
                ::close(m_fd);
            }
        }
        [[nodiscard]] bool valid() const
        {
            return m_fd >= 0;
        }
        [[nodiscard]] int fd() const
        {
            return m_fd;
        }
    };
}

/**
 * Compare two files, that should both exist
 * @param src
 * @param dest
 * @return
 */
static bool are_identical(const path_t& src, const path_t& dest)
{
    AutoFd srcFile{::open(src.c_str(), O_RDONLY)};
    AutoFd destFile{::open(dest.c_str(), O_RDONLY)};
    assert(srcFile.valid());
    assert(destFile.valid());

    struct stat srcStat{};
    struct stat destStat{};

    auto srcRet = ::fstat(srcFile.fd(), &srcStat);
    assert(srcRet == 0); std::ignore = srcRet;
    auto destRet = ::fstat(destFile.fd(), &destStat);
    assert(destRet == 0); std::ignore = destRet;

    if (srcStat.st_size != destStat.st_size)
    {
        // Different sizes means they are different...
        return false;
    }
    constexpr int BUFFER_SIZE = 1024*1024;

    char srcBuffer[BUFFER_SIZE];
    char destBuffer[BUFFER_SIZE];

    while (true)
    {
        auto srcRead = ::read(srcFile.fd(), srcBuffer, BUFFER_SIZE);
        auto destRead = ::read(destFile.fd(), destBuffer, BUFFER_SIZE);
        if (srcRead != destRead)
        {
            return false;
        }
        if (srcRead <= 0)
        {
            break;
        }
        assert(srcRead <= BUFFER_SIZE);
        auto cmpRet = memcmp(srcBuffer, destBuffer, srcRead);
        if (cmpRet != 0)
        {
            return false;
        }
    }

    return true;
}

static bool copyFile(const path_t& src, const path_t& dest)
{
    std::error_code ec;
    fs::remove(dest, ec); // Don't worry if the destination doesn't exist
    fs::copy(src, dest, fs::copy_options::overwrite_existing, ec);
    if (ec)
    {
        PRINT("Failed to copy "<<src.string()<< " to "<< dest.string());
        return false;
    }
    return true;
}

int sync_versioned_files::copy(const path_t& src, const path_t& dest)
{
    // for each file in dest, work out what we expect as a source, and see if it is present
    if (fs::is_directory(dest))
    {
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
                // Files different so copy
                if (!copyFile(src_name, dest_name))
                {
                    return 1;
                }
            }
        }
    }
    else
    {
        fs::create_directories(dest);
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
            if (!copyFile(src_name, dest_name))
            {
                return 2;
            }
        }
    }
    return 0;
}
