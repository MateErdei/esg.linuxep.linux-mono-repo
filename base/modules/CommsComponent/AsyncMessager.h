/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once
#include <string>
#include <functional>
#include <deque>
#include <atomic>


#include <boost/asio.hpp>
using namespace boost::asio;
using MessageReceivedCB = std::function<void(std::string)>; 
using MessageQueue = std::deque<std::string>;
namespace Comms
{       

    class AsyncMessager{
        public:
        static constexpr size_t  capacity = 4095;
        static constexpr size_t  bufferSize = capacity + 1;  
        static constexpr char FinalChunk = '0'; 
        static constexpr char PartialChunk = '1'; 

            AsyncMessager(boost::asio::io_service& io,
                local::datagram_protocol::socket && socket,
                MessageReceivedCB onNewMessage );
            
            void sendMessage(const std::string&);
            void push_stop();

        private:
            void read();
            void do_write();

        boost::asio::io_service & m_io;
        boost::asio::strand<boost::asio::io_context::executor_type> m_strand;
        local::datagram_protocol::socket m_socket;
        MessageReceivedCB m_onNewMessage;
        std::mutex m_mutex;
        MessageQueue m_queue; 
        std::array<char,bufferSize> buffer;
        std::vector<std::string> m_pendingChunks;        
        int count;
    };

    struct CommsContext{
        static std::pair<std::unique_ptr<AsyncMessager>, std::unique_ptr<AsyncMessager>> setupPairOfConnectedSockets(boost::asio::io_service& io_service, 
                        MessageReceivedCB onMessageReceivedFirst, MessageReceivedCB onMessageReceivedSecond ); 
        static std::thread startThread(boost::asio::io_service& io_service); 
    };
}
