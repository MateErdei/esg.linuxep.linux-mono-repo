/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <Common/UtilityImpl/StringUtils.h>
#include "ManifestEntry.h"

using namespace Installer::ManifestDiff;

ManifestEntry::ManifestEntry(std::string path)
    : m_size(0),m_path(toPosixPath(path))
{
}

ManifestEntry& ManifestEntry::withSHA1(const std::string& hash)
{
    m_sha1 = hash;
    return *this;
}

ManifestEntry& ManifestEntry::withSHA256(const std::string& hash)
{
    m_sha256 = hash;
    return *this;
}

ManifestEntry& ManifestEntry::withSHA384(const std::string& hash)
{
    m_sha384 = hash;
    return *this;
}

std::string ManifestEntry::sha1() const
{
    return m_sha1;
}

std::string ManifestEntry::sha256() const
{
    return m_sha256;
}

std::string ManifestEntry::sha384() const
{
    return m_sha384;
}

std::string ManifestEntry::path() const
{
    return m_path;
}

std::string ManifestEntry::toPosixPath(const std::string& p)
{
    std::string result = Common::UtilityImpl::StringUtils::replaceAll(p,
            "\\",
            "/");
    if (Common::UtilityImpl::StringUtils::startswith(result,"./"))
    {
        result = result.substr(2);
    }
    return result;
}

ManifestEntry& ManifestEntry::withSize(unsigned long size)
{
    m_size = size;
    return *this;
}

unsigned long ManifestEntry::size() const
{
    return m_size;
}

bool ManifestEntry::operator<(const ManifestEntry& other) const
{
    if (m_path < other.m_path)
    {
        return true;
    }
    if (m_size < other.m_size)
    {
        return true;
    }
    return m_sha1 < other.m_sha1;
}
