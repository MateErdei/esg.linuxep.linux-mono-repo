/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once
#include "Configurator.h"
#include "MessageChannel.h"
#include "ReactorAdapter.h"
#include "AsyncMessager.h"
#include <functional>
#include <unistd.h>
#include <log4cplus/logger.h>

namespace CommsComponent
{
    /** The splitProcess must be called when only one thread is running in the parent process. 
     *  The flow is: 
     *  
     *   . Setup the socket pair
     *   . Fork
     *   . In the Parent: 
     *   .   Arm monitor of the child to notify onChildExited
     *   .   Arm the reactor for child Messages
     *   .   Setup ChildService
     *   .   Run Config::applyParentSecurityPolicies
     *   .   Run Config::applyParentInit (usually where things like log4cpp will be initialized)
     *   .   Run ParentExecutor::run   
     *   .   Teardown and return.
     *   . In the child:
     *   .   Arm the reactor for parent messages
     *   .   Setup ParentService
     *   .   Run Config::applyChildSecurityPolicies
     *   .   Run Config::applyChildInit
     *   .   Run ChildExecutor::run
     *   .   Teardown and exit
     * 
     *   The reason for enforcing that the ChildExecutor must be passed as an rvalue is because it intends to be 'consumed' 
     *   as any operation on it will happen in the 'to be created process' and no side effect will be seen by the caller of splitProcesses. 
     * 
     * Hence, usually, applications will do: 
     *  main(xxx){
     *    auto config = createConfig(); 
     *    auto parentExecutor = createParentExecutor(); 
     *    auto childExecutor = createChildExecutor(); 
     *    return splitProcesses(parentExecutor, std::move(childExecutor), config); 
     * }
     * 
     *  This is the main function. Other functions below are built on top of this one and 
     *   exists to provide simpler ways to interact with the splitProcesss.
     * */
    template<typename ParentExecutor, typename ChildExecutor, typename Config>
    int splitProcesses( ParentExecutor parent, ChildExecutor && child, Config config)
    {
        boost::asio::io_service io_service;

        auto [parentChannel, childChannel] = CommsContext::setupPairOfConnectedSockets(
        io_service, [&parent](std::string message){ parent.onMessageFromOtherSide(std::move(message)); },
        [&child](std::string message){ child.onMessageFromOtherSide(std::move(message));}); 

        log4cplus::Logger::shutdown();
        io_service.notify_fork(boost::asio::io_context::fork_prepare);
        int exitCode = 0; 
        std::cout << "Parent pid: << " << getpid() << std::endl; 
        auto pid = fork();         
        assert(pid != -1); 
        if ( pid == 0)
        {
            io_service.notify_fork(boost::asio::io_context::fork_child);
            parentChannel->justShutdownSocket(); 
            {
                OtherSideApi parentService{std::move(childChannel)}; 

                config.applyChildSecurityPolicy(); 
                config.applyChildInit();         
                std::thread thread = CommsContext::startThread(io_service);
                std::cout << "Child run" << std::endl; 
                exitCode = child.run(parentService); 
                std::cout << "Child run passed" << std::endl; 
                parentService.close(); 
                thread.join();            
                log4cplus::Logger::shutdown(); 
            }
            std::cout << "after join" << std::endl; 
            std::cout << "Child finished" << std::endl; 
            exit(exitCode);

        }
        else
        {
            io_service.notify_fork(boost::asio::io_context::fork_parent);
            boost::asio::signal_set signal_{io_service, SIGCHLD};
            signal_.async_wait(
                [](boost::system::error_code ec, int /*signo*/)
                {
                    std::cout << ec << std::endl; 
                // Reap completed child processes so that we don't end up with
                // zombies.
                std::cout << "wait pid: " << getpid() << std::endl; 
                int status = 0;
                while (waitpid(-1, &status, WNOHANG) > 0) {}
                });             
            childChannel->justShutdownSocket(); 
            OtherSideApi childService(std::move(parentChannel));
            config.applyParentSecurityPolicy(); 
            config.applyChildInit(); 
            
            std::thread thread = CommsContext::startThread(io_service);            
            
            std::cout << "Parent run service" << std::endl; 
            exitCode = parent.run(childService); 
            childService.close();
            std::cout << "Parent Wait on child" << std::endl; 
            
            int status; 
            // FIXME: 
            // unfortunatelly log4cplus is still hanging in the destructor because it uses static objects and 
            // we have to figure out how to properly work with it. 
            // without the kill below, the executable will be hanging on the child, 
            // child stack: 
            //   std::condition_variable::wait(std::unique_lock<std::mutex>&) () from /usr/lib/x86_64-linux-gnu/libstdc++.so.6
            //   log4cplus::(anonymous namespace)::destroy_default_context::~destroy_default_context() ()
            //   ...log4cplus/lib/liblog4cplus-2.0.so.3
            //    __cxa_finalize (d=0x7f4872552000) at cxa_finalize.c:83
            

            kill(pid, SIGHUP);

            wait(&status); 
            std::cout << "parent after wait" << std::endl; 
            log4cplus::Logger::shutdown();
            thread.join();
            std::cout << "parent finished" << std::endl; 
            // wait on child

        }
        return exitCode; 
    } 

    /* Creates a ChildExcutor that tunnel the messages received by the Parent into a 
    *  MessageChannel which throw ChannelClosed when the Parent sends a stop message. 
    *  The functor does not need to capture that exception, it is handled by the childexecutor
    *  to teardown the childprocess. 
    *  It is usefull for cases where the child process does little more than reacting to messages 
    *   received by the parent. 
    */

    template<typename ParentExecutor,typename Config>
    int splitProcessesSimpleReactor(ParentExecutor parent, SimpleReactor child, Config config )
    {
        return splitProcesses(parent, ReactorAdapter{child, "child"}, config); 
    }

    /* 
    Further simplification of splitProcessesSimpleReactor to provide the NullConfigurator as config. 
    It is likely to be used only in tests. But, given that it is only a template function, no code is 
    generated in production if not used, hence, no harm to have it here. 
    */
    template<typename ParentExecutor>    
    int splitProcessesSimpleReactor(ParentExecutor parent, SimpleReactor child )
    {
        return splitProcessesSimpleReactor(parent, ReactorAdapter{child}, NullConfigurator{}); 
    }

    template<typename Config>
    int splitProcessesReactors(SimpleReactor parent, SimpleReactor child, Config config )
    {
        return splitProcesses(ReactorAdapter{parent, "parent"}, ReactorAdapter{child, "child"}, config); 
    }

    /* Further simplification of splitProcessesReactors to pass NullConfigurator as config. 
       Again, likely to be used only in tests. But no harm to describe here as it will not go into production code anyway. 
    */
    int splitProcessesReactors(SimpleReactor parent, SimpleReactor child)
    {
        return splitProcessesReactors( parent, child, NullConfigurator{}); 
    }



} // namespace CommsComponent

