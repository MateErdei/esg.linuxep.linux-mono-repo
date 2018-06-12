//
// Created by pair on 11/06/18.
//

#ifndef EVEREST_BASE_POLLERIMPL_H
#define EVEREST_BASE_POLLERIMPL_H


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

        class PollerImpl
            : public virtual ZeroMQWrapper::IPoller
        {
        public:
            ZeroMQWrapper::IPoller::poll_result_t poll(long timeoutMs);
            ZeroMQWrapper::IPoller::poll_result_t poll(const std::chrono::milliseconds& timeout) override;

            void addEntry(ZeroMQWrapper::IHasFD &entry, PollDirection directionMask) override;

            std::unique_ptr<ZeroMQWrapper::IHasFD> addEntry(int fd, PollDirection directionMask) override;
        private:
            std::vector<PollEntry> m_entries;
        };
    }
}

#endif //EVEREST_BASE_POLLERIMPL_H
