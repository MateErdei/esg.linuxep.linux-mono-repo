/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once
#include <string>
#include <functional>
#include <deque>


#include <boost/asio.hpp>
using namespace boost::asio;
using MessageReceivedCB = std::function<void(std::string)>; 
using MessageQueue = std::deque<std::string>;
namespace Comms
{       

    class AsyncMessager{
        public:
            AsyncMessager(boost::asio::io_service& io, 
                local::stream_protocol::socket && socket, 
                MessageReceivedCB onNewMessage );
            
            void sendMessage(std::string);
            void shutdown();

// not to be used directly
            void handle_read(const boost::system::error_code& ec, std::size_t size); 
            void do_write(); 
        private:
        boost::asio::io_service & m_io; 
        local::stream_protocol::socket m_socket; 
        MessageReceivedCB m_onNewMessage;
        MessageQueue m_queue; 
        std::array<char,512> buffer; 
    };

    struct CommsContext{
        boost::asio::io_service m_io; 
        std::pair<AsyncMessager, AsyncMessager> setupPairOfConnectedSockets(MessageReceivedCB onMessageReceivedFirst, MessageReceivedCB onMessageReceivedSecond ); 
        void run(); 
    };
}
