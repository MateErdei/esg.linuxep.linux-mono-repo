//
// Created by pair on 11/06/18.
//

#include "PollerImpl.h"
#include "SocketImpl.h"
#include "ZeroMQWrapperException.h"

#include <Common/Exceptions/Print.h>

#include <zmq.h>
#include <cassert>
#include <climits>

using Common::ZeroMQWrapperImpl::PollerImpl;

Common::ZeroMQWrapper::IPollerPtr Common::ZeroMQWrapper::createPoller()
{
    return Common::ZeroMQWrapper::IPollerPtr(new PollerImpl());
}


std::vector<Common::ZeroMQWrapper::IHasFD*> Common::ZeroMQWrapperImpl::PollerImpl::poll(long timeoutMs)
{
    std::vector<ZeroMQWrapper::IHasFD*> results;

    std::unique_ptr<zmq_pollitem_t[]> items(new zmq_pollitem_t[m_entries.size()]);
    assert(items.get() != nullptr); // NOLINT

    // If performance of this code becomes a problem we could save the poll set, and re-use it.
    assert(m_entries.size() < INT_MAX);
    auto size = static_cast<int>(m_entries.size());

    for (int i=0; i<size; ++i)
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
        throw ZeroMQWrapperException("Failed to poll");
    }

    for (size_t i=0; i<m_entries.size(); ++i)
    {
        // FIXME - Need to handle the case where there is an error condition present on socket specified by the fd.
        // Potentially need to remove bad fd. Note this is rarely seen. See ticket LINUXEP-6155
        if (items[i].revents != 0 && items[i].revents != ZMQ_POLLERR)
        {
            results.push_back(m_entries[i].entry);
        }
    }

    return results;
}


Common::ZeroMQWrapper::IPoller::poll_result_t PollerImpl::poll(const std::chrono::milliseconds& timeout)
{
    return poll(timeout.count());
}

void Common::ZeroMQWrapperImpl::PollerImpl::addEntry(Common::ZeroMQWrapper::IHasFD &entry, PollDirection directionMask)
{
    PollEntry pollentry{&entry,directionMask};
    m_entries.push_back(pollentry);
}

namespace
{
    class FDWrapper :
        public virtual Common::ZeroMQWrapper::IHasFD
    {
    public:
        explicit FDWrapper(int fd)
            : m_fd(fd)
        {}

        FDWrapper() = delete;
        FDWrapper(const FDWrapper&) = delete;

        int fd() override
        {
            return m_fd;
        }

        int m_fd;
    };
}

std::unique_ptr<Common::ZeroMQWrapper::IHasFD> Common::ZeroMQWrapperImpl::PollerImpl::addEntry(int fd, PollDirection directionMask)
{
    std::unique_ptr<ZeroMQWrapper::IHasFD> fdWrapper(new FDWrapper(fd));

    addEntry(*fdWrapper,directionMask);

    return fdWrapper;
}
