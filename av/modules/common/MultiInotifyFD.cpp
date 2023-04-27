// Copyright 2023 Sophos All rights reserved.
#include "MultiInotifyFD.h"

#include <cassert>

#include <sys/inotify.h>


common::MultiInotifyFD::MultiInotifyFD() : m_inotifyFD(inotify_init())
{
    assert(m_inotifyFD.valid());
}

int common::MultiInotifyFD::watch(const sophos_filesystem::path& path, uint32_t mask)
{
    return inotify_add_watch(m_inotifyFD.fd(), path.c_str(), mask);
}

void common::MultiInotifyFD::unwatch(int watchFD)
{
    inotify_rm_watch(m_inotifyFD.fd(), watchFD);
}
