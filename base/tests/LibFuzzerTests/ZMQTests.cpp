/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

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

            m_replier->setTimeout(10000);
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
                    auto * replierImpl = dynamic_cast<Common::ZeroMQWrapperImpl::SocketImpl*>(m_replier.get());
                    if( replierImpl)
                    {
                        replierImpl->refresh();
                    }

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
    // the normal exchange, the replier accepts the greeting
    std::array<char, 100> bf;
    int rc = recv(socketfd, bf.data(), bf.size(),0);
    if( rc <0)
    {
        int er=errno;
        logErr("Requester: Failed to receive answer from replier. " + info, er);
    }
}


#ifdef HasLibFuzzer
DEFINE_PROTO_FUZZER(const  ZMQPartsProto::ZMQStack & message) {
#else
void mainTest(const  ZMQPartsProto::ZMQStack & message){
#endif
    static ReplierRun replierRun;
    assert(message.identity()[0]==4);
    /*
    std::this_thread::sleep_for(std::chrono::seconds(100));

    Common::ZMQWrapperApi::IContextSharedPtr m_contextPtr{Common::ZMQWrapperApi::createContext()};

    Common::ZeroMQWrapper::ISocketRequesterPtr m_req{m_contextPtr->getRequester()};

    std::string socket_address = std::string("ipc://") + IPCADDRESS;

    m_req->setTimeout(300);
    m_req->connect(socket_address);
    m_req->write({"req"});
    return ;
    */


    int s;
    struct sockaddr_un remote_addr = {};

    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        int err=errno;
        logErr("Failed to create socket. ", err);
        abort();
    }
    SocketGuard socketGuard{s};
    remote_addr.sun_family = AF_UNIX;
    std::string addr=IPCADDRESS;
    memcpy( remote_addr.sun_path, addr.c_str(), addr.size());
    int count = 0;
    int rc = -1;
    while( count++ < 10  && rc == -1)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        rc = connect(s, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr_un));
    }

    if (rc == -1)
    {
        int err=errno;
        logErr("Failed to connect socket. ", err);

        abort();
    }

    if ( !send_data_to_socket(s, message.greeting()))
    {
        log("Give up at greeting stage");
        // it is ok to fail to send this data.
        return;
    }
    // the normal exchange, the replier accepts the greeting
    receive_data_from_replier(s, "Greeting");



    if( !send_data_to_socket(s, message.handshake()))
    {
        log("Give up at handshake stage");
        return;
    }

    receive_data_from_replier(s, "Handshake");


    if( !send_data_to_socket(s, message.identity()))
    {
        log("Give up at identity stage");
        return;
    }
    receive_data_from_replier(s, "Identity");

    if ( !send_data_to_socket(s, message.payload()))
    {
        log("Give up at payload stage");
        return;
    }
    receive_data_from_replier(s, "Answer");
    log("Success");

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

    mainTest(message);

    return 0;
}

#endif

