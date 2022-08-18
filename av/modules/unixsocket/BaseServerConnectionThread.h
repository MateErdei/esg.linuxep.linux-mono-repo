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
        [[nodiscard]] bool isRunning() const;
    protected:
        void setIsRunning(bool value);
    private:
        std::atomic_bool m_isRunning = false;
    };
}