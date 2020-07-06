/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "AsyncMessager.h"
#include <iostream>
#include <boost/bind.hpp>
using namespace boost::asio;
namespace Comms
{
    AsyncMessager::AsyncMessager(boost::asio::io_service& io, 
        local::stream_protocol::socket && socket, 
        MessageReceivedCB onNewMessage )
    : 
        m_io(io), 
        m_socket(std::move(socket)),
        m_onNewMessage(onNewMessage),
        m_queue(),
        buffer()
    {
       // Wait for request.
        m_socket.async_read_some(boost::asio::buffer(buffer),
            boost::bind(&AsyncMessager::handle_read,
            this, boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));

    }

    void AsyncMessager::handle_read(const boost::system::error_code& ec, std::size_t size)
    {
        if (!ec)
        {
            
            std::string m{buffer.begin(), buffer.begin()+size}; 
            //this->m_onNewMessage(m);
            m_socket.async_read_some(boost::asio::buffer(buffer),
            boost::bind(&AsyncMessager::handle_read,
            this, boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
        }
        else
        {
            throw boost::system::system_error(ec);
        }
    }

    void AsyncMessager::sendMessage(std::string message)
    {

        boost::asio::write(socket, boost::asio::buffer(request));

        // std::cout << "send message" << std::endl; 
        // boost::asio::post(m_io,
        //     [this, message]()
        // {
        //   bool write_in_progress = !m_queue.empty();
        //   m_queue.push_back("tests");
        //   if (!write_in_progress)
        //   {
        //     do_write();
        //   }
        // });
        // std::cout << "message scheculed" << std::endl; 
    }
    
    void AsyncMessager::do_write()
    {
        std::cout << "do write test: " << m_queue.front()  << "and lenght: " << m_queue.front().length()<< std::endl; 
        m_socket.async_write_some(boost::asio::buffer(m_queue.front().data(),
                m_queue.front().length()),
        [this](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec)
          {
            m_queue.pop_front();
            if (!m_queue.empty())
            {
              do_write();
            }
          }
          else
          {
              throw boost::system::system_error(ec); 
          }
          std::cout << "send message with success" << std::endl; 
        });   
        std::cout << "write done" << std::endl;      
    }

    void AsyncMessager::shutdown()
    {
        m_socket.close(); 
    }

    std::pair<AsyncMessager, AsyncMessager> CommsContext::setupPairOfConnectedSockets(MessageReceivedCB onMessageReceivedFirst, MessageReceivedCB onMessageReceivedSecond )
    {
        
        local::stream_protocol::socket socket1(m_io);
        local::stream_protocol::socket socket2(m_io);
        local::connect_pair(socket1, socket2);

        return std::make_pair(AsyncMessager{m_io, std::move(socket1), onMessageReceivedFirst}, 
            AsyncMessager{m_io, std::move(socket2), onMessageReceivedSecond} ); 
    }

    void CommsContext::run()
    {
        try{
            m_io.run(); 
        }catch(std::exception & ex)
        {
            std::cerr << "Exception in thread: " << ex.what() << "\n";
        }
        
    }
}

