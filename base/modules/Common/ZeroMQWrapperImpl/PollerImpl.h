/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/ZeroMQWrapper/IPoller.h>

namespace Common
{
    namespace ZeroMQWrapperImpl
    {
        struct PollEntry
        {
            ZeroMQWrapper::IHasFD* entry;
            ZeroMQWrapper::IPoller::PollDirection pollMask;
        };

        class PollerImpl : public virtual ZeroMQWrapper::IPoller
        {
        public:
            ZeroMQWrapper::IPoller::poll_result_t poll(long timeoutMs);
            ZeroMQWrapper::IPoller::poll_result_t poll(const std::chrono::milliseconds& timeout) override;

            void addEntry(ZeroMQWrapper::IHasFD& entry, PollDirection directionMask) override;

            std::unique_ptr<ZeroMQWrapper::IHasFD> addEntry(int fd, PollDirection directionMask) override;

        private:
            std::vector<PollEntry> m_entries;
        };
    } // namespace ZeroMQWrapperImpl
} // namespace Common
