/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "common/AbstractThreadPluginInterface.h"

#include <atomic>

namespace unixsocket
{
    class BaseServerConnectionThread : public common::AbstractThreadPluginInterface
    {
    public:
        BaseServerConnectionThread(const BaseServerConnectionThread&) = delete;
        BaseServerConnectionThread& operator=(const BaseServerConnectionThread&) = delete;

        explicit BaseServerConnectionThread(const std::string& threadName);
        [[nodiscard]] bool isRunning() const;

    protected:
        void setIsRunning(bool value);
        const std::string m_threadName;

    private:
        std::atomic_bool m_isRunning = false;
    };
}