// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "PollerImpl.h"

#include "SocketImpl.h"
#include "ZeroMQWrapperException.h"

#include "modules/Common/Exceptions/Print.h"

#include <cassert>
#include <climits>
#include <zmq.h>

using Common::ZeroMQWrapperImpl::PollerImpl;

std::vector<Common::ZeroMQWrapper::IHasFD*> Common::ZeroMQWrapperImpl::PollerImpl::poll(long timeoutMs)
{
    std::vector<ZeroMQWrapper::IHasFD*> results;

    std::unique_ptr<zmq_pollitem_t[]> items(new zmq_pollitem_t[m_entries.size()]);
    assert(items.get() != nullptr); // NOLINT

    // If performance of this code becomes a problem we could save the poll set, and re-use it.
    assert(m_entries.size() < INT_MAX);
    auto size = static_cast<int>(m_entries.size());

    for (int i = 0; i < size; ++i)
    {
        // See if we have a 0MQ socket, if so then poll that instead
        auto socket = dynamic_cast<SocketImpl*>(m_entries[i].entry);
        if (socket == nullptr)
        {
            items[i].socket = nullptr;
            items[i].fd = m_entries[i].entry->fd();
        }
        else
        {
            items[i].socket = socket->skt();
            items[i].fd = -1;
        }
        items[i].events = 0;
        items[i].revents = 0;
        PollDirection mask = m_entries[i].pollMask;
        if (mask & Common::ZeroMQWrapper::IPoller::POLLIN)
        {
            items[i].events |= ZMQ_POLLIN;
        }

        if (mask & Common::ZeroMQWrapper::IPoller::POLLOUT)
        {
            items[i].events |= ZMQ_POLLOUT;
        }
    }

    int rc = zmq_poll(items.get(), size, timeoutMs);

    if (rc == 0)
    {
        return results;
    }
    if (rc < 0)
    {
        // error case
        if (errno == EINTR)
        {
            return results;
        }
        std::string info = "Failed to poll. ";
        int z_err = zmq_errno();
        if (z_err != 0)
        {
            info += zmq_strerror(z_err);
        }

        throw ZeroMQPollerException(info);
    }

    for (size_t i = 0; i < m_entries.size(); ++i)
    {
        if (items[i].revents != 0)
        {
            if (items[i].revents & ZMQ_POLLIN)
            {
                results.push_back(m_entries[i].entry);
            }
            else if (items[i].revents & ZMQ_POLLERR)
            {
                std::string info = "Error while polling associated to file descriptor ";
                info += std::to_string(items[i].fd);
                throw ZeroMQPollerException(info);
            }
            else
            {
                assert(false);
                throw ZeroMQPollerException("Case not expected.");
            }
        }
    }

    return results;
}

Common::ZeroMQWrapper::IPoller::poll_result_t PollerImpl::poll(const std::chrono::milliseconds& timeout)
{
    return poll(timeout.count());
}

void Common::ZeroMQWrapperImpl::PollerImpl::addEntry(Common::ZeroMQWrapper::IHasFD& entry, PollDirection directionMask)
{
    PollEntry pollentry{ &entry, directionMask };
    m_entries.push_back(pollentry);
}

namespace
{
    class FDWrapper : public virtual Common::ZeroMQWrapper::IHasFD
    {
    public:
        explicit FDWrapper(int fd) : m_fd(fd) {}

        FDWrapper() = delete;
        FDWrapper(const FDWrapper&) = delete;

        int fd() override { return m_fd; }

        int m_fd;
    };
} // namespace

std::unique_ptr<Common::ZeroMQWrapper::IHasFD> Common::ZeroMQWrapperImpl::PollerImpl::addEntry(
    int fd,
    PollDirection directionMask)
{
    std::unique_ptr<ZeroMQWrapper::IHasFD> fdWrapper(new FDWrapper(fd));

    addEntry(*fdWrapper, directionMask);

    return fdWrapper;
}
