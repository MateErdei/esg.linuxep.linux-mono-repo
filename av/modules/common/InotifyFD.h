// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "datatypes/AutoFd.h"
#include "datatypes/sophos_filesystem.h"

namespace common
{
    class InotifyFD
    {
    public:
        explicit InotifyFD(const sophos_filesystem::path& directory);

        ~InotifyFD();
        InotifyFD(const InotifyFD&) = delete;
        InotifyFD& operator=(const InotifyFD&) = delete;

        int getFD();

        [[nodiscard]] bool valid() const
        {
            return m_watchDescriptor >= 0;
        }

    private:
        datatypes::AutoFd m_inotifyFD;
        int m_watchDescriptor;
    };
}
