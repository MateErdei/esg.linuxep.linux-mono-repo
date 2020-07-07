/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "AsyncMessager.h"
#include "Logger.h"
#include <iostream>
#include <boost/bind.hpp>

using namespace boost::asio;
namespace Comms
{
    AsyncMessager::AsyncMessager(boost::asio::io_service& io, 
        local::datagram_protocol::socket && socket,
        MessageReceivedCB onNewMessage )
    : 
        m_io(io),
        m_strand(boost::asio::make_strand(m_io)),
        m_socket(std::move(socket)),
        m_onNewMessage(onNewMessage),
        m_queue(),
        buffer(),
        count{0}
    {
       read();
    }
    void AsyncMessager::read()
    {
        std::cout << "arm reader" << std::endl;
        m_socket.async_receive(boost::asio::buffer(buffer),
         boost::asio::bind_executor(m_strand,
             [this](boost::system::error_code ec, std::size_t size)
             {
                std::cout << "finished read" << std::endl;
                 if (!ec)
                 {
                     std::string message{buffer.begin(), buffer.begin()+size};
                     this->m_onNewMessage(message);
                     std::cout << message << std::endl;
                     read();
               }
               else
               {
                   if ( ec.value() !=  boost::asio::error::operation_aborted)
                   {
                       std::cout << "error category : " << ec.category().name()  << "value: " << ec.value() << std::endl;
                       throw boost::system::system_error(ec);
                   }
               }
             })
             );

    }

    void AsyncMessager::sendMessage(const std::string& message)
    {
//        m_socket.send(boost::asio::buffer(message));

         std::cout << "send message" << std::endl;
         boost::asio::post(m_io,
                           boost::asio::bind_executor(m_strand,
             [this, message]()
         {
           bool write_in_progress = !m_queue.empty();
           m_queue.push_back(message);
           if (!write_in_progress)
           {
             do_write();
           }
         }));
         std::cout << "message scheculed" << std::endl;
    }
    
    void AsyncMessager::do_write()
    {
        std::cout << "do write test: " << m_queue.front()  << "and lenght: " << m_queue.front().length()<< std::endl; 
        m_socket.async_send(boost::asio::buffer(m_queue.front().data(),
                m_queue.front().length()),
                            boost::asio::bind_executor(m_strand,
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
        }));
        std::cout << "write done" << std::endl;      
    }

    void AsyncMessager::push_stop()
    {
         boost::asio::post(m_io,
                           boost::asio::bind_executor(m_strand,
             [this]()
         {
             m_socket.close();
         }));
    }

    std::pair<std::unique_ptr<AsyncMessager>, std::unique_ptr<AsyncMessager>> CommsContext::setupPairOfConnectedSockets(boost::asio::io_service& io_service, 
            MessageReceivedCB onMessageReceivedFirst, MessageReceivedCB onMessageReceivedSecond )
    {
        local::datagram_protocol::socket s1(io_service);
        local::datagram_protocol::socket s2(io_service);

        local::connect_pair(s1, s2);

        return std::make_pair(std::make_unique<AsyncMessager>(io_service, std::move(s1), onMessageReceivedFirst),
            std::make_unique<AsyncMessager>(io_service, std::move(s2), onMessageReceivedSecond ));                            
    }

    std::thread CommsContext::startThread(boost::asio::io_service& io_service)
    {
         return std::thread{[&io_service]()
         {
           try
           {
             io_service.run();
           }
           catch (std::exception& ex)
           {
            LOGERROR( "Exception in thread: " << ex.what() ); 
           }
         }
         }; 
    }
}

