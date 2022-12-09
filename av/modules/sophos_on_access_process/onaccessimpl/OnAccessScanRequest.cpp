// Copyright 2022, Sophos Limited.  All rights reserved.

#include "OnAccessScanRequest.h"

#include "Logger.h"

#include "common/SaferStrerror.h"

#include <boost/functional/hash.hpp>

#include <stdexcept>

using sophos_on_access_process::onaccessimpl::OnAccessScanRequest;

OnAccessScanRequest::unique_t OnAccessScanRequest::uniqueMarker() const
{
    if (!fstatIfRequired())
    {
        throw std::runtime_error("Unable to create unique value - fstat failed");
    }

    return unique_t{m_fstat.st_dev, m_fstat.st_ino};
}

std::optional<OnAccessScanRequest::hash_t> OnAccessScanRequest::hash() const
{
    if (!fstatIfRequired())
    {
        return {};
    }

    constexpr std::hash<unsigned long> hasher;

    const hash_t h1 = hasher(m_fstat.st_dev);
    const hash_t h2 = hasher(m_fstat.st_ino);

    hash_t seed{};
    boost::hash_combine(seed, h1);
    boost::hash_combine(seed, h2);
    return seed;
}

bool OnAccessScanRequest::fstatIfRequired() const
{
    if (!m_autoFd.valid())
    {
        return false;
    }
    if (!m_syscalls)
    {
        return false;
    }
    if (m_fstat.st_dev == 0 && m_fstat.st_ino == 0)
    {
        int ret = m_syscalls->fstat(m_autoFd.get(), &m_fstat);
        if (ret != 0)
        {
            LOGERROR("Unable to stat: " << m_path << " error " << common::safer_strerror(errno) << " (" << errno << ")");
            return false;
        }
    }
    return true;
}

bool OnAccessScanRequest::operator==(const OnAccessScanRequest& other) const
{
    return (other.m_fstat.st_dev == m_fstat.st_dev &&
            other.m_fstat.st_ino == m_fstat.st_ino);
}
