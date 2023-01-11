#include <iostream>
#include <sstream>
#include <cassert>
#include <string>
#include <thread>
#include "modules/unixsocket/BaseClient.h"
#include "modules/unixsocket/SocketUtils.h"
// Generate AV event
//  EventPubSub -s /opt/sophos-spl/plugins/av/var/threatEventPublisherSocketPath send

namespace
{
    class TestClient : public unixsocket::BaseClient
    {
    public:
        explicit TestClient(
            std::string socket_path,
            const BaseClient::duration_t& sleepTime= std::chrono::seconds{1},
            BaseClient::IStoppableSleeperSharedPtr sleeper={}) :
            BaseClient(std::move(socket_path), sleepTime, std::move(sleeper))
        {
            unixsocket::BaseClient::connectWithRetries("SafeStore Rescan");
        }

        void sendRequest(std::string request)
        {
            assert(m_socket_fd.valid());
            try
            {
                if (!unixsocket::writeLengthAndBuffer(m_socket_fd.get(), request))
                {
                    std::stringstream errMsg;
                    errMsg << "Failed to write rescan request to socket [" << errno << "]";
                    throw std::runtime_error(errMsg.str());
                }
            }
            catch (unixsocket::environmentInterruption& e)
            {
                std::cerr << "Failed to write to SafeStore Rescan socket. Exception caught: " << e.what();
            }
        }
    };
} // namespace