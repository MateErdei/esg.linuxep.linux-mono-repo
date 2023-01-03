// Copyright 2023 Sophos Limited. All rights reserved.

#include "StatusFile.h"

#include "datatypes/sophos_filesystem.h"

#include <utility>

#include <fcntl.h>
#include <unistd.h>

using namespace common;

StatusFile::StatusFile(std::string path)
    : m_path(std::move(path))
{
    disabled();
}

namespace
{
    const std::string ENABLED{"enabled"};
    const std::string DISABLED{"disabled"};
}

void StatusFile::enabled()
{
    m_fd.reset(::open(m_path.c_str(), O_CREAT | O_WRONLY));
    ::write(m_fd.get(), ENABLED.c_str(), ENABLED.size());
    // Add locking if required
}

void StatusFile::disabled()
{
    m_fd.reset(::open(m_path.c_str(), O_CREAT | O_WRONLY));
    ::write(m_fd.get(), DISABLED.c_str(), DISABLED.size());
}

bool StatusFile::isEnabled(const std::string& path)
{
    datatypes::AutoFd fd{::open(path.c_str(), O_RDONLY)};
    if (!fd.valid())
    {
        return false;
    }
    // check locking if required
    char value;
    auto count = ::read(fd.get(), &value, 1);
    if (count != 1)
    {
        return false;
    }
    return (value == 'e');
}

StatusFile::~StatusFile()
{
    m_fd.reset();
    ::unlink(m_path.c_str());
}
