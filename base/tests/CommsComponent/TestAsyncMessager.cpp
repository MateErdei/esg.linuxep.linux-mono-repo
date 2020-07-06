/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <modules/CommsComponent/AsyncMessager.h>

using namespace Comms; 


struct MessagesAppender
{
    std::vector<std::string> & m_messages; 
    MessagesAppender(std::vector<std::string> & messages): m_messages(messages){}
    void operator()(std::string newMessage){
        m_messages.emplace_back(std::move(newMessage)); 
    }
};

TEST(TestAsyncMessager, MessagesCanBeInterchangedByAsyncMessager) // NOLINT
{
    using ListStrings = std::vector<std::string>; 
    ListStrings firstReceivedMessages; 
    ListStrings secondReceivedMessages; 

    {
        CommsContext context; 
        auto [firstM, secondM] = context.setupPairOfConnectedSockets(MessagesAppender{firstReceivedMessages}, MessagesAppender{secondReceivedMessages}); 

        firstM.sendMessage("fromFirst1");
        // firstM.sendMessage("fromFirst2");
        // secondM.sendMessage("fromSecond1");
        // secondM.sendMessage("fromSecond2");
        context.run(); 
        std::cout << "after run" << std::endl; 
        secondM.shutdown();
        std::cout << "finished" << std::endl; 
        std::this_thread::sleep_for(std::chrono::milliseconds(300)); 
    }
    ListStrings expectedFirst{{"fromSecond1"}, {"fromSecond2"}}; 
    EXPECT_EQ(firstReceivedMessages, expectedFirst);
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
