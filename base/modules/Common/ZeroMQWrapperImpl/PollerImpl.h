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
            std::vector<ZeroMQWrapper::IHasFD*> poll(long timeoutMs) override;

            void addEntry(ZeroMQWrapper::IHasFD &entry, PollDirection directionMask) override;

            std::unique_ptr<ZeroMQWrapper::IHasFD> addEntry(int fd, PollDirection directionMask) override;
        private:
            std::vector<PollEntry> m_entries;
        };
    }
}

#endif //EVEREST_BASE_POLLERIMPL_H
