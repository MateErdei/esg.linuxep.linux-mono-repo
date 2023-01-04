// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "datatypes/AutoFd.h"
#include "datatypes/sophos_filesystem.h"

namespace common
{
    class InotifyFD
    {
    public:
        InotifyFD() = default;

        virtual ~InotifyFD();
        InotifyFD(const InotifyFD&) = delete;
        InotifyFD& operator=(const InotifyFD&) = delete;

        int getFD();

        [[nodiscard]] bool valid() const
        {
            return m_watchDescriptor >= 0;
        }

    protected:
        datatypes::AutoFd m_inotifyFD;
        int m_watchDescriptor = -1;
    };
}
