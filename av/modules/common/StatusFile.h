// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "datatypes/AutoFd.h"

#include <string>

namespace common
{
    class StatusFile
    {
    public:
        explicit StatusFile(std::string path);
        virtual ~StatusFile();
        void enabled();
        void disabled();

        static bool isEnabled(const std::string& path);
    private:
        const std::string m_path;
        datatypes::AutoFd m_fd;
    };
}
