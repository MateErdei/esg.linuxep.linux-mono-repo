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

    class SigUSR1Monitor : public IMonitorable
    {
    public:
        SigUSR1Monitor() = delete;
        SigUSR1Monitor(const SigUSR1Monitor&) = delete;
        explicit SigUSR1Monitor(IReloadablePtr reloadable);
        ~SigUSR1Monitor() override;

        int monitorFd() override;
        void triggered() override;

    private:
        Common::Threads::NotifyPipe m_pipe;
        IReloadablePtr m_reloader;
    };
}