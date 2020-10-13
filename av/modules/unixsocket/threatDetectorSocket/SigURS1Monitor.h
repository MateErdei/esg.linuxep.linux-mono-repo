/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <unixsocket/IMonitorable.h>

#include <Common/Threads/NotifyPipe.h>

namespace unixsocket
{
    class IReloadable
    {
    public:
        virtual void reload() = 0;
        virtual ~IReloadable() = default;
    };
    using IReloadablePtr = std::shared_ptr<IReloadable>;

    class SigURS1Monitor : public IMonitorable
    {
    public:
        SigURS1Monitor() = delete;
        SigURS1Monitor(const SigURS1Monitor&) = delete;
        explicit SigURS1Monitor(IReloadablePtr reloadable);
        ~SigURS1Monitor() override;

        int monitorFd() override;
        void triggered() override;

    private:
        Common::Threads::NotifyPipe m_pipe;
        IReloadablePtr m_reloader;
    };
}