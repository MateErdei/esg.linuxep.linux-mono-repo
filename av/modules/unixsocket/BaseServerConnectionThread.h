// Copyright 2020-2022 Sophos Limited. All rights reserved.

#pragma once

#include "common/AbstractThreadPluginInterface.h"

#include <atomic>

namespace unixsocket
{
    class BaseServerConnectionThread : public common::AbstractThreadPluginInterface
    {
    public:
        BaseServerConnectionThread() = default;
        BaseServerConnectionThread(const BaseServerConnectionThread&) = delete;
        BaseServerConnectionThread& operator=(const BaseServerConnectionThread&) = delete;

        [[nodiscard]] bool isRunning() const;
    protected:
        void setIsRunning(bool value);
    private:
        std::atomic_bool m_isRunning{ false };
    };
}