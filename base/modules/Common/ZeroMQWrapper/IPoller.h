//
// Created by pair on 11/06/18.
//

#ifndef EVEREST_BASE_IPOLLER_H
#define EVEREST_BASE_IPOLLER_H

#include "IHasFD.h"

#include <memory>
#include <vector>
#include <chrono>

namespace Common
{
    namespace ZeroMQWrapper
    {
        class IPoller
        {
        public:
            enum PollDirection
            {
                POLLIN = 1,
                POLLOUT = 2
            };

            virtual ~IPoller() = default;

            using poll_result_t = std::vector<IHasFD*>;

            /**
             *
             * Wait to see if any of the provided sockets or FD is ready to action.
             * @param timeout, 0 to return immediately, -1 to wait forever, otherwise the timeout
             * @return Vector of BORROWED IHasFD pointers that are ready to action
             */
            virtual poll_result_t poll(const std::chrono::milliseconds& timeout) = 0;

            /**
             * Add a socket to the poller.
             * @param entry
             */
            virtual void addEntry(IHasFD &entry, PollDirection directionMask) = 0;

            /**
             * Return a new IHasFD* for a provided file descriptor.
             * @param fd
             * @return DONATED IHasFD
             */
            virtual IHasFDPtr addEntry(int fd, PollDirection directionMask) = 0;
        };

        using IPollerPtr = std::unique_ptr<IPoller>;

        /**
         * Factory function to create a Poller.
         * @return DONATED new Poller
         */
        extern IPollerPtr createPoller();
    }
}
#endif //EVEREST_BASE_IPOLLER_H
