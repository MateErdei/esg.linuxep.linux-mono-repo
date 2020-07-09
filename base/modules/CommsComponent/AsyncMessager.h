/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once
#include <boost/asio.hpp>

#include <atomic>
#include <deque>
#include <functional>
#include <string>
using namespace boost::asio;
using MessageReceivedCB = std::function<void(std::string)>;
using NofifySocketClosed = std::function<void()>; 
using MessageQueue = std::deque<std::string>;
namespace CommsComponent
{
    class AsyncMessager
    {
    public:
        static constexpr size_t Capacity = 4095;
        static constexpr size_t BufferSize = Capacity + 1;
        static constexpr char FinalChunk = '0';
        static constexpr char PartialChunk = '1';
        static const std::string& StopMessage()
        {
            static std::string stop{ "stop" };
            return stop;
        }

        AsyncMessager(
            boost::asio::io_service& io,
            local::datagram_protocol::socket&& socket,
            MessageReceivedCB onNewMessage);

        AsyncMessager(const AsyncMessager&) = delete;
        AsyncMessager& operator=(const AsyncMessager&) = delete;

        /* throw if the string is empty as empty string is an invalid message to exchange
            Message may fail to be delivered, but because it sends the message asynchronously, this can only be 
            seen from the io_service. 
        */
        void sendMessage(const std::string&);

        void setNotifyClosure(NofifySocketClosed ); 

        /* Schedule an asynchronous request to stop the communication. 
           It will send a stop message to the other size to allow it to close as well. 
           */

        void push_stop();


        void justShutdownSocket();

    private:
        void read();
        void do_write();
        void deliverMessage(std::string&& message);

        boost::asio::io_service& m_io;
        boost::asio::strand<boost::asio::io_context::executor_type> m_strand;
        local::datagram_protocol::socket m_socket;
        MessageReceivedCB m_onNewMessage;
        NofifySocketClosed m_notifyClosureDetected; 
        std::mutex m_mutex;
        MessageQueue m_queue;
        std::array<char, BufferSize> buffer;
        std::vector<std::string> m_pendingChunks;
        int count;
    };

    struct CommsContext
    {
        static std::pair<std::unique_ptr<AsyncMessager>, std::unique_ptr<AsyncMessager>> setupPairOfConnectedSockets(
            boost::asio::io_service& io_service,
            MessageReceivedCB onMessageReceivedFirst,
            MessageReceivedCB onMessageReceivedSecond);
        static std::thread startThread(boost::asio::io_service& io_service);
    };
} // namespace Comms
