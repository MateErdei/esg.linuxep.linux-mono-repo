// Copyright 2023 Sophos Limited. All rights reserved.

#include "StatusFile.h"

#include "Logger.h"
#include "SaferStrerror.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <utility>
#include <tuple>

using namespace common;

StatusFile::StatusFile(std::string path)
    : m_path(std::move(path))
{
    disabled();
}

namespace
{
    const std::string ENABLED {"enabled\n"};
    const std::string DISABLED{"disabled\n"};
}

bool StatusFile::open()
{
    // Not opening with O_TRUNC so that we don't have a case where we have an empty file
    // The first char changes atomically
    m_fd.reset(::open(m_path.c_str(), O_CREAT | O_WRONLY | O_CLOEXEC, 0644));
    if (!m_fd.valid())
    {
        LOGERROR("Failed to create status file at " << m_path << ": " << common::safer_strerror(errno));
        return false;
    }
    ::fchmod(m_fd.get(), 0644);
    return true;
}

void StatusFile::enabled()
{
    if (!open())
    {
        return;
    }

    auto written = ::write(m_fd.get(), ENABLED.c_str(), ENABLED.size());
    if (written < 0)
    {
        LOGERROR("Failed to write status file at " << m_path << ": " << common::safer_strerror(errno));
    }
    std::ignore = ftruncate(m_fd.get(), ENABLED.size());
    fsync(m_fd.get());

    // Add locking if required
}

void StatusFile::disabled()
{
    if (!open())
    {
        return;
    }

    auto written = ::write(m_fd.get(), DISABLED.c_str(), DISABLED.size());
    if (written < 0)
    {
        LOGERROR("Failed to write status file at " << m_path << ": " << common::safer_strerror(errno));
    }
    std::ignore = ftruncate(m_fd.get(), DISABLED.size());
    fsync(m_fd.get());
}

bool StatusFile::isEnabled(const std::string& path)
{
    datatypes::AutoFd fd{::open(path.c_str(), O_RDONLY | O_CLOEXEC)};
    if (!fd.valid())
    {
        LOGDEBUG("Failed to open status file " << path);
        return false;
    }
    // check locking if required
    char value;
    auto count = ::read(fd.get(), &value, 1);
    if (count != 1)
    {
        LOGDEBUG("Failed to read status file " << path);
        return false;
    }
    LOGDEBUG("Read " << value << " from status file " << path);
    return (value == 'e');
}

StatusFile::~StatusFile()
{
    disabled();
}