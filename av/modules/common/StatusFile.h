// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include <string>

namespace common
{
    class StatusFile
    {
    public:
        explicit StatusFile(std::string path);
        void enabled();
        void disabled();

        static bool isEnabled(const std::string& path);
    private:
        const std::string m_path;
    };
}
