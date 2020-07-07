/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <modules/CommsComponent/AsyncMessager.h>
#include <boost/thread/thread.hpp>
#include <tests/Common/Helpers/TestExecutionSynchronizer.h>
using namespace Comms; 

struct MessagesAppender
{
    std::vector<std::string> & m_messages;
    Tests::TestExecutionSynchronizer & m_synchronizer;
    MessagesAppender(std::vector<std::string> & messages,
            Tests::TestExecutionSynchronizer & synchronizer):
            m_messages(messages), m_synchronizer(synchronizer){}
    void operator()(std::string newMessage){
        m_messages.emplace_back(std::move(newMessage));
        m_synchronizer.notify();
    }
};

//sendMessage(str)
//str + '1' or '0'
//break_str_into_chunks( 4095) (4096)
//'stop'
//
//
//recv(buffer)
//check last (0): combine previous messages
//if (1) append to a pending_parts



TEST(TestAsyncMessager, MessagesCanBeInterchangedByAsyncMessager) // NOLINT
{
    using ListStrings = std::vector<std::string>;
    boost::asio::io_service m_io;
    local::datagram_protocol::socket socket1(m_io);
    local::datagram_protocol::socket socket2(m_io);
    local::connect_pair(socket1, socket2);
    ListStrings firstReceivedMessages1;
    ListStrings firstReceivedMessages2;
    Tests::TestExecutionSynchronizer synchronizer(3);
    AsyncMessager m1{m_io, std::move(socket1), MessagesAppender{firstReceivedMessages1, synchronizer} };
    AsyncMessager m2{m_io, std::move(socket2), MessagesAppender{firstReceivedMessages2, synchronizer} };
    std::thread thread(
         [&m_io]()
         {
           try
           {
             m_io.run();
           }
           catch (std::exception& e)
           {
             std::cerr << "Exception in thread: " << e.what() << "\n";}
         });

    std::string message{"basictest"};
    std::string message2{"basictest2"};
    std::string fromm1{"from_m1"};
    m1.sendMessage(fromm1);
    m2.sendMessage(message);
    m2.sendMessage(message2);
    EXPECT_TRUE(synchronizer.waitfor(1000));
    m1.push_stop();
    m2.push_stop();
    thread.join();
    ASSERT_EQ(firstReceivedMessages1.size(), 2);
    ASSERT_EQ(firstReceivedMessages2.size(), 1);
    EXPECT_EQ(message, firstReceivedMessages1.at(0));
    EXPECT_EQ(message2, firstReceivedMessages1.at(1));
    EXPECT_EQ(fromm1, firstReceivedMessages2.at(0));

//
//    ListStrings secondReceivedMessages;
//
//    {
//        CommsContext context;
//        auto pair = context.setupPairOfConnectedSockets(MessagesAppender{firstReceivedMessages}, MessagesAppender{secondReceivedMessages});
//
//        pair.first.sendMessage("fromFirst1");
//        // firstM.sendMessage("fromFirst2");
//        pair.second.sendMessage("fromSecond1");
//        // secondM.sendMessage("fromSecond2");
//        std::thread thread(
//         [&context]()
//         {
//           try
//           {
//             context.m_io.run();
//           }
//           catch (std::exception& e)
//           {
//             std::cerr << "Exception in thread: " << e.what() << "\n";
//             std::exit(1);
//           }
//         });
//        //context.run();
//        thread.join();
//
//        std::cout << "after run" << std::endl;
//        pair.second.shutdown();
//        std::cout << "finished" << std::endl;
//        std::this_thread::sleep_for(std::chrono::milliseconds(300));
//    }
//    ListStrings expectedFirst{{"fromSecond1"}, {"fromSecond2"}};
//    EXPECT_EQ(firstReceivedMessages, expectedFirst);
}


TEST(TestAsyncMessager, SendReceiveDataGram) // NOLINT
{
    using ListStrings = std::vector<std::string>;
    boost::asio::io_service m_io;
    local::datagram_protocol::socket socket1(m_io);
    local::datagram_protocol::socket socket2(m_io);
    local::connect_pair(socket1, socket2);
    ListStrings firstReceivedMessages;
    std::thread thread(
            [&m_io]() {
                try {
                    m_io.run();
                }
                catch (std::exception &e) {
                    std::cerr << "Exception in thread: " << e.what() << "\n";
                }
            });

    std::thread thread2(
            [&socket1, &firstReceivedMessages](){
                std::array<char,1024> buffer;
                auto size = socket1.receive(boost::asio::buffer(buffer));
                firstReceivedMessages.push_back(std::string{buffer.begin(), buffer.begin()+size});
                size = socket1.receive(boost::asio::buffer(buffer));
                firstReceivedMessages.push_back(std::string{buffer.begin(), buffer.begin()+size});
            }
            );
    std::string message{"basictest"};
    std::string message2{"basictest"};

    socket2.send(boost::asio::buffer(message));
    socket2.send(boost::asio::buffer(message2));
    thread.join();
    thread2.join();
    ASSERT_EQ(firstReceivedMessages.size(), 2);
    EXPECT_EQ(message, firstReceivedMessages.at(0));
    EXPECT_EQ(message2, firstReceivedMessages.at(1));
}

// #include <array>
// #include <iostream>
// #include <string>
// #include <cctype>
// #include <boost/asio.hpp>
// #include <boost/thread/thread.hpp>

// using boost::asio::local::stream_protocol;

// class read_background
// {
// public:
//   read_background(stream_protocol::socket sock)
//     : socket_(std::move(sock))
//   {
//     read();
//   }

// //private:
//   void read()
//   {
//     socket_.async_read_some(boost::asio::buffer(data_),
//         [this](boost::system::error_code ec, std::size_t size)
//         {
//           if (!ec)
//           {
//             // // Compute result.
//             // for (std::size_t i = 0; i < size; ++i)
//             //   data_[i] = std::toupper(data_[i]);


//             std::cout << "read_background: " << std::string{data_.begin(), data_.begin()+size} << std::endl; 
//             read(); 
//             //write(size);
//           }
//           else
//           {
//             throw boost::system::system_error(ec);
//           }
//         });
//   }

  
//   void write(std::string request)
//   {
//     boost::asio::write(socket_, boost::asio::buffer(request));
//   }

//   stream_protocol::socket socket_;
//   std::array<char, 512> data_;
// };




// class uppercase_filter
// {
// public:
//   uppercase_filter(stream_protocol::socket sock)
//     : socket_(std::move(sock))
//   {
//     read();
//   }

// private:
//   void read()
//   {
//     socket_.async_read_some(boost::asio::buffer(data_),
//         [this](boost::system::error_code ec, std::size_t size)
//         {
//           if (!ec)
//           {
//             // Compute result.
//             for (std::size_t i = 0; i < size; ++i)
//               data_[i] = std::toupper(data_[i]);


//             std::cout << "up: " << std::string{data_.begin(), data_.begin()+size} << std::endl; 

//             read(); 
//             write(size); 
//           }
//           else
//           {
//             throw boost::system::system_error(ec);
//           }
//         });
//   }

//   void write(std::size_t size)
//   {
//     boost::asio::async_write(socket_, boost::asio::buffer(data_, size),
//         [this](boost::system::error_code ec, std::size_t /*size*/)
//         {
//           if (!ec)
//           {
//             // Wait for request.
//             //read();
//           }
//           else
//           {
//             throw boost::system::system_error(ec);
//           }
//         });
//   }

//   stream_protocol::socket socket_;
//   std::array<char, 512> data_;
// };

// int main()
// {
//   try
//   {
//     boost::asio::io_context io_context;

//     // Create a connected pair and pass one end to a filter.
//     stream_protocol::socket socket(io_context);
//     stream_protocol::socket filter_socket(io_context);
//     boost::asio::local::connect_pair(socket, filter_socket);
//     uppercase_filter filter(std::move(filter_socket));

//     // The io_context runs in a background thread to perform filtering.
//     boost::thread thread(
//         [&io_context]()
//         {
//           try
//           {
//             io_context.run();
//           }
//           catch (std::exception& e)
//           {
//             std::cerr << "Exception in thread: " << e.what() << "\n";
//             std::exit(1);
//           }
//         });

//     read_background reader{std::move(socket)}; 
//     while (std::cin.good())
//     {
//       // Collect request from user.
//       std::cout << "Enter a string: ";
//       std::string request;
//       std::getline(std::cin, request);

//       // Send request to filter.

//       reader.write(request);   



//       /*boost::asio::write(socket, boost::asio::buffer(request));

//       // Wait for reply from filter.
//       std::vector<char> reply(request.size());
//       boost::asio::read(socket, boost::asio::buffer(reply));

//       // Show reply to user.
//       std::cout << "Result: ";
//       std::cout.write(&reply[0], request.size());
//       std::cout << std::endl;
//       */
//     }
//   }
//   catch (std::exception& e)
//   {
//     std::cerr << "Exception: " << e.what() << "\n";
//     std::exit(1);
//   }
// }
