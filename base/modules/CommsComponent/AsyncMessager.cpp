/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "AsyncMessager.h"

#include "Logger.h"

#include <boost/bind.hpp>

#include <iostream>

using namespace boost::asio;
using namespace boost::placeholders;
namespace CommsComponent
{
    AsyncMessager::AsyncMessager(
        boost::asio::io_service& io,
        local::datagram_protocol::socket&& socket,
        MessageReceivedCB onNewMessage) :
        m_io(io),
        m_strand(boost::asio::make_strand(m_io)),
        m_socket(std::move(socket)),
        m_onNewMessage(onNewMessage),
        m_notifyClosureDetected([](){}),
        m_queue(),
        buffer(),
        count{ 0 }
    {
        read();
    }

    /*
     * Periodically read messages from the socket and send them to the 'asynchronous callback'.
     * One important thing about this method is related to handling 'chunks' of messages (or fragmented messages).
     * The reason for this is that datagrams ( the socket used here ) impose some limits on the ammount of
     * data that can be transfered in a single transfer. But, from the high level point of view, we want to provide
     * an interface that allows for a string of any size to be inserted and the 'same' string to 'appear' in the other
     * side.
     *
     * see the ::sendMessage method for detail on how the message is 'broken' into chunks to be sent.
     *
     */
    void AsyncMessager::read()
    {
        m_socket.async_receive(
            boost::asio::buffer(buffer),
            boost::asio::bind_executor(m_strand, [this](boost::system::error_code ec, std::size_t size) {                
                if (!ec)
                {
                    LOGDEBUG("AsyncMessager new message received: " << size << " bytes");
                    // the chunk is the full message minus the last byte (control byte.)
                    std::string chunk{ buffer.begin(), buffer.begin() + size - 1 };
                    char controlByte = buffer.at(size - 1);
                    // control byte is FinalChunk?
                    if (controlByte == AsyncMessager::FinalChunk)
                    {
                        if (this->m_pendingChunks.empty())
                        {
                            deliverMessage(std::move(chunk));
                        }
                        else
                        {
                            std::string message;
                            message.reserve((m_pendingChunks.size() + 1) * Capacity);
                            for (auto& m : this->m_pendingChunks)
                            {
                                message += m;
                            }
                            message += chunk;
                            deliverMessage(std::move(message));
                        }
                        m_pendingChunks.clear();
                    }
                    else if (controlByte == AsyncMessager::PartialChunk)
                    {
                        m_pendingChunks.push_back(chunk);
                    }
                    else if (StopMessage() == (chunk + controlByte))
                    {
                        m_notifyClosureDetected(); 
                        return; // request to stop does not rearm the read.
                    }
                    else
                    {
                        LOGINFO("Unexpected message with " << size << " bytes. Ignoring it.");
                    }

                    read();
                }
                else
                {
                    LOGINFO("AsyncMessager closing reading operation");
                    if (ec.value() != boost::asio::error::operation_aborted)
                    {
                        LOGDEBUG("AsyncMessager error on reading message error category : " << ec.category().name() << "value: " << ec.value() << ec.message()) ;
                        throw boost::system::system_error(ec);
                    }
                }
            }));
    }

    void AsyncMessager::setNotifyClosure(NotifySocketClosed notifySocketClosed)
    {
        m_notifyClosureDetected = notifySocketClosed;
    }

    void AsyncMessager::deliverMessage(std::string&& message)
    {
        try
        {
            this->m_onNewMessage(std::move(message));
        }
        catch (std::exception& ex)
        {
            LOGWARN("Callback on new message throw exception: " << ex.what());
        }
    }

    /**  Schedule a message to be sent via the io_service.
     *
     *   The message is broken into chunks of AsyncMessager::Capacity.
     *   If the message is the last one (or if the message is smaller than the capacity) the message will be sent with a
     * control character AsyncMessager::FinalChunk Otherwise, the message will be sent with a control character called
     * AsyncMessager::PartialChunk.
     *
     *   The reason to choose the last character as the control character and not the first one, is that, normally,
     * messages will be smaller than the capacity, which means that only a small character will be appended to the
     * message (which usually does not force an allocation). While if it were the first character, than an extra
     * allocation would be necessary for each and every message.
     *
     *
     *
     * */
    void AsyncMessager::sendMessage(const std::string& message)
    {

        if (message.empty())
        {
            throw std::runtime_error("Empty message can not be exchanged"); 
        }

        // the mutex is to ensure that the message broken into chunks are scheduled in the correct order inside the
        // io_service even if the AsyncMessage were to be used in different threads.
        std::lock_guard<std::mutex> lo{ m_mutex };
        size_t sent = 0;
        while (sent < message.size())
        {
            size_t lowerBound = sent;
            size_t higherBound = std::min(lowerBound + Capacity, message.size());
            std::string chunk{ message.begin() + lowerBound, message.begin() + higherBound };
            if (higherBound == message.size())
            {
                chunk += AsyncMessager::FinalChunk;
            }
            else
            {
                chunk += AsyncMessager::PartialChunk;
            }
            boost::asio::post(m_io, boost::asio::bind_executor(m_strand, [this, chunk]() {
                                  bool write_in_progress = !m_queue.empty();
                                  m_queue.push_back(chunk);
                                  if (!write_in_progress)
                                  {
                                      do_write();
                                  }
                              }));
            sent = higherBound;
        }
    }

    void AsyncMessager::do_write()
    {
        m_socket.async_send(
            boost::asio::buffer(m_queue.front().data(), m_queue.front().length()),
            boost::asio::bind_executor(m_strand, [this](boost::system::error_code ec, std::size_t /*length*/) {
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
                    if (ec.value() != boost::asio::error::connection_refused && ec.value() != boost::asio::error::bad_descriptor)
                    {
                        LOGDEBUG("AsyncMessager error on writting message error category : " << ec.category().name() << "value: " << ec.value() << ec.message()) ;                        
                        throw boost::system::system_error(ec);
                    }
                    LOGSUPPORT("Other side closed communication channel");
                }
            }));
    }

    void AsyncMessager::push_stop()
    {
        std::lock_guard<std::mutex> lo{ m_mutex };
        boost::asio::post(m_io, boost::asio::bind_executor(m_strand, [this]() {
                              bool write_in_progress = !m_queue.empty();
                              m_queue.push_back(StopMessage());
                              if (!write_in_progress)
                              {
                                  do_write();
                              }
                          }));

        boost::asio::post(m_io, boost::asio::bind_executor(m_strand, [this]() { m_socket.close(); }));
    }

    void AsyncMessager::justShutdownSocket()
    {
        std::lock_guard<std::mutex> lo{ m_mutex };
        boost::asio::post(m_io, boost::asio::bind_executor(m_strand, [this]() { m_socket.close(); }));
    }

    std::pair<std::unique_ptr<AsyncMessager>, std::unique_ptr<AsyncMessager>> CommsContext::setupPairOfConnectedSockets(
        boost::asio::io_service& io_service,
        MessageReceivedCB onMessageReceivedFirst,
        MessageReceivedCB onMessageReceivedSecond)
    {
        local::datagram_protocol::socket s1(io_service);
        local::datagram_protocol::socket s2(io_service);

        local::connect_pair(s1, s2);

        return std::make_pair(
            std::make_unique<AsyncMessager>(io_service, std::move(s1), onMessageReceivedFirst),
            std::make_unique<AsyncMessager>(io_service, std::move(s2), onMessageReceivedSecond));
    }

    std::thread CommsContext::startThread(boost::asio::io_service& io_service)
    {
        return std::thread{ [&io_service]() {
            try
            {
                io_service.run();
            }
            catch (std::exception& ex)
            {
                LOGERROR("Exception in thread: " << ex.what());
            }
        } };
    }
} // namespace Comms
