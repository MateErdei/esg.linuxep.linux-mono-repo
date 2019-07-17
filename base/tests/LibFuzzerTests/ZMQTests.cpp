/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

 Documentation and helper to developers.
 ZMQTests goal is to stress the socket layer that we rely on to transmit data accros the plugins and modules.
 Other fuzzer like ManagementAgentApiTest verifies our logic by applying a valid zmq and protobuf message.
 This fuzzer tries to explore the different layers of zmq.
 It is not enough to just send 'garbage' data as the zmq employs a handshake protocol.
 In order to detect the handshake associated, we create two programs:

 : server
     Common::ZMQWrapperApi::IContextSharedPtr m_contextPtr{Common::ZMQWrapperApi::createContext()};
     Common::ZeroMQWrapper::ISocketReplierPtr m_replier{m_contextPtr->getReplier()};
     std::string socket_address = std::string("ipc:///tmp/test.ipc");
     m_replier->setTimeout(500);
     m_replier->listen(socket_address);
     m_replier->read()


 : client
    Common::ZMQWrapperApi::IContextSharedPtr m_contextPtr{Common::ZMQWrapperApi::createContext()};
    Common::ZeroMQWrapper::ISocketRequesterPtr m_req{m_contextPtr->getRequester()};
    std::string socket_address = std::string("ipc:///tmp/test.ipc");
    m_req->setTimeout(300);
    m_req->connect(socket_address);
    m_req->write({"req"});

 And patched the zmq library with the following patch:
diff --git a/src/tcp.cpp b/src/tcp.cpp
index e79c84d6..2f1b4311 100644
--- a/src/tcp.cpp
+++ b/src/tcp.cpp
@@ -238,6 +238,13 @@ int zmq::tcp_write (fd_t s_, const void *data_, size_t size_)
return nbytes;

#else
+       const char * buff = static_cast<const char *>(data_);
+
+       for(int i=0; i<size_; i++)
+       {
+               std::cout << "0x" << std::hex << (int) buff[i]  << std::dec << ", ";
+       }
+       std::cout << "// -> " << size_ << " bytes" << std::endl;
ssize_t nbytes = send (s_, static_cast<const char *> (data_), size_, 0);


 Build zmq locally.
 By running the client with LD_LIBRARY_PATH set to the path of the zmq library changed.
 It will print messages like:
 0x4, 0x19, 0x5, 0x52, 0x45, 0x41, 0x44, 0x59, 0xb, 0x53, 0x6f, 0x63, 0x6b, 0x65, 0x74, 0x2d, 0x54, 0x79, 0x70, 0x65, 0x0, 0x0, 0x0, 0x3, 0x52, 0x45, 0x50, // -> 27 bytes
 Applying a small python manipulation:
 >>> l=[ 0x4, 0x19, 0x5, 0x52, 0x45, 0x41, 0x44, 0x59, 0xb, 0x53, 0x6f, 0x63, 0x6b, 0x65, 0x74, 0x2d, 0x54, 0x79, 0x70, 0x65, 0x0, 0x0, 0x0, 0x3, 0x52, 0x45, 0x50]
 >>> ''.join([chr(e) for e in l])
 \x04\x19\x05READY\x0bSocket-Type\x00\x00\x00\x03REP


 The ZMQTests also applies the same techinique of ManagementAgentApiTest to enable building and running on
 the developer IDE as well as in the machine that will run the fuzzer.

 To know how to operate the fuzzer check: https://llvm.org/docs/LibFuzzer.html

******************************************************************************************************/

#include "FuzzerUtils.h"
#include "zmqparts.pb.h"
#include "google/protobuf/text_format.h"
#include <modules/Common/ZeroMQWrapper/ISocketRequester.h>
#include <modules/Common/ZeroMQWrapper/ISocketReplier.h>
#include <modules/Common/ZMQWrapperApi/IContextSharedPtr.h>
#include <modules/Common/ZeroMQWrapperImpl/SocketReplierImpl.h>
#include <modules/Common/ZMQWrapperApi/IContext.h>
#include <modules/Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <modules/Common/FileSystem/IFileSystem.h>
#ifdef  HasLibFuzzer
#include <libprotobuf-mutator/src/mutator.h>
#include <libprotobuf-mutator/src/libfuzzer/libfuzzer_macro.h>
#endif
#include <future>
#include <thread>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <modules/Common/ZeroMQWrapperImpl/SocketImpl.h>
#include <modules/Common/ZeroMQWrapperImpl/SocketUtil.h>
#include <modules/Common/ZeroMQWrapper/IIPCException.h>
#include <Common/ZeroMQWrapperImpl/PollerImpl.h>
#include <cassert>
#include <zmq.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/un.h>
#include <stdint.h>
#include <stddef.h>

namespace
{
    const char* IPCADDRESS = "/tmp/zqmtest.ipc";


    void s_signal_handler(int)
    {

        exit(0);

    };

    std::atomic<bool> GL_log{false};

    void log( const std::string & info)
    {
        if ( GL_log)
        {
            std::cout << info + '\n';
        }

    }
    void logErr(const std::string & info, int errno_=0)
    {
        if ( GL_log)
        {
            std::string append;
            if( errno_ != 0)
            {
                append+= ::strerror(errno_);
            }
            std::cerr << info + append + '\n';
        }

    }

}



class ReplierRun : public Runner
{

public:
    ReplierRun()
    {
        setMainLoop([]()
        {

            struct sigaction action;
            action.sa_handler = s_signal_handler;
            action.sa_flags = 0;
            sigemptyset(&action.sa_mask);
            sigaction(SIGINT, &action, NULL);
            sigaction(SIGTERM, &action, NULL);

            Common::ZMQWrapperApi::IContextSharedPtr m_contextPtr{Common::ZMQWrapperApi::createContext()};

            Common::ZeroMQWrapper::ISocketReplierPtr m_replier{m_contextPtr->getReplier()};

            std::string socket_address = std::string("ipc://") + IPCADDRESS;

            m_replier->setTimeout(500);
            m_replier->listen(socket_address);
            while(true)
            {
                try
                {
                    auto request = m_replier->read();
                    if ( !request.empty())
                    {
                        log( "Replier: request -> " + request[0] );
                    }
                    m_replier->write(request);
                }
                catch ( Common::ZeroMQWrapper::IIPCException& ex )
                {
                    logErr(std::string("Replier: ZMQ exception -> ") + ex.what());
                }
                catch( std::exception &ex )
                {
                    logErr(std::string("Replier: Non expected exception -> ") + ex.what());
                    ::abort();
                }
            }

        });
    }

};


class SocketGuard
{
    int m_fd;
public :
    SocketGuard( int fd): m_fd{fd}{}
    ~SocketGuard()
    {
        ::close(m_fd);
    }
};


bool send_data_to_socket( int socketfd, const std::string & payload)
{
    std::string sendinfo= "Send data: " + std::to_string(payload.size()) + " bytes";
    log(sendinfo);
    if ( payload.empty())
    {
        //nothing to send.
        return true;
    }

    int rc = send(socketfd, payload.data(), payload.size(), 0);
    if( rc < 0)
    {
        int err = errno;
        logErr("Requester: Failed to send data via socket", err);
        return false;
    }
    return true;
}

void receive_data_from_replier(int socketfd, const std::string & info)
{
    Common::ZeroMQWrapperImpl::PollerImpl poller;

    // the normal exchange, the replier accepts the greeting
    std::array<char, 100> bf;
    int rc = 0;

    auto entry= poller.addEntry(socketfd, Common::ZeroMQWrapper::IPoller::PollDirection::POLLIN);

    auto poller_result = poller.poll(1000);
    if( poller_result.empty())
    {
        logErr("Requester: Timeout -> " , errno);

    }
    else
    {
        rc = recv(socketfd, bf.data(), bf.size(),0);
    }

    if( rc <0)
    {
        int er=errno;
        logErr("Requester: Failed to receive answer from replier. " + info, er);
    }
    std::string received = ".Received bytes: ";
    received += std::to_string(rc);
    log(info + received);
}

class AnounceFinished
{
public:
    AnounceFinished() = default;
    ~AnounceFinished()
    {
        log("Main test finished");
    }
};

void sendMessageToReplier(const  ZMQPartsProto::ZMQStack & message, const std::string addr)
{
    int socketFileDescriptor;
    struct sockaddr_un remote_addr = {};

    if ((socketFileDescriptor = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        int err=errno;
        logErr("Failed to create socket. ", err);
        abort();
    }
    SocketGuard socketGuard{socketFileDescriptor};
    remote_addr.sun_family = AF_UNIX;
    memcpy( remote_addr.sun_path, addr.c_str(), addr.size());
    int count = 0;
    int rc = -1;
    while( count++ < 10  && rc == -1)
    {
        rc = connect(socketFileDescriptor, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr_un));
        if( rc != 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    }

    if (rc == -1)
    {
        int err=errno;
        logErr("Failed to connect socket. ", err);

        abort();
    }
    log("Sending data to Replier");
    if ( !send_data_to_socket(socketFileDescriptor, message.greeting()))
    {
        log("Give up at greeting stage");
        // it is ok to fail to send this data.
        return;
    }
    // the normal exchange, the replier accepts the greeting
    receive_data_from_replier(socketFileDescriptor, "Greeting");



    if( !send_data_to_socket(socketFileDescriptor, message.handshake()))
    {
        log("Give up at handshake stage");
        return;
    }

    receive_data_from_replier(socketFileDescriptor, "Handshake");


    if( !send_data_to_socket(socketFileDescriptor, message.identity()))
    {
        log("Give up at identity stage");
        return;
    }


    if ( !send_data_to_socket(socketFileDescriptor, message.payload()))
    {
        log("Give up at payload stage");
        return;
    }
    receive_data_from_replier(socketFileDescriptor, "Answer");
    log("Success");
}


#ifdef HasLibFuzzer
DEFINE_PROTO_FUZZER(const  ZMQPartsProto::ZMQStack & message) {
#else
void mainTest(const  ZMQPartsProto::ZMQStack & message){
#endif
    static ReplierRun replierRun;
    AnounceFinished anounceFinished;

    sendMessageToReplier(message, IPCADDRESS);

}

#ifndef HasLibFuzzer
int main(int argc, char *argv[])
{
    GL_log =true;
    if ( argc < 2)
    {
        logErr("Invalid argument. Usage: ZMQTests <input_file>. File must be of protobuf ZMQStack");
        return 1;
    }
    std::string content = Common::FileSystem::fileSystem()->readFile(argv[1]);
    google::protobuf::TextFormat::Parser parser;
    parser.AllowPartialMessage(true);
    ZMQPartsProto::ZMQStack message;
    if( !parser.ParseFromString(content, &message))
    {
        logErr("Invalid file. Failed to parse its contents. " + content);
        return 3;
    }

    if( argc==3 )
    {
        // this is to enable sending to other ipc channels.
        std::string ipcaddr = argv[2];
        if( ipcaddr.find(".ipc") != std::string::npos)
        {
            log("Sending message to : " + ipcaddr);
            sendMessageToReplier(message, ipcaddr);
            return 0;
        }
    }

    mainTest(message);

    return 0;
}

#endif

