/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Configurator.h"
#include "MessageChannel.h"
#include "ReactorAdapter.h"
#include "AsyncMessager.h"
#include "Logger.h"
#include <modules/Common/Logging/FileLoggingSetup.h>
#include <log4cplus/logger.h>
#include <functional>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>


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
     *   .   Run ParentExecutor::run   
     *   .   Teardown and return.
     *   . In the child:
     *   .   Arm the reactor for parent messages
     *   .   Setup ParentService
     *   .   Run Config::applyChildSecurityPolicies
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
    int splitProcesses(ParentExecutor parent, ChildExecutor&& child, Config config)
    {
        // setup the io_service and the socket/pair.
        boost::asio::io_service io_service;
        auto[parentChannel, childChannel] = CommsContext::setupPairOfConnectedSockets(
                io_service, [&parent](std::string message) { parent.onMessageFromOtherSide(std::move(message)); },
                [&child](std::string message) { child.onMessageFromOtherSide(std::move(message)); });

        // shutdown all threads before fork.

        io_service.notify_fork(boost::asio::io_context::fork_prepare);
        int exitCode = 0;

        auto pid = fork();
        if (pid == -1)
        {
            perror("Comms component failed to start");
            exit(EXIT_FAILURE);
        }
        if (pid == 0)
        {
            // the io_service thread in the background to receive messages
            std::thread thread;
            try
            {
                // child code
                io_service.notify_fork(boost::asio::io_context::fork_child);
                // Close the file descriptor of the socket that is not supposed to be used.
                parentChannel->justShutdownSocket();

                OtherSideApi parentService{std::move(childChannel)};

                // Apply the Configurator.
                config.applyChildSecurityPolicy();

                LOGINFO("Entering main process: pid " << getpid());
                thread = CommsContext::startThread(io_service);
                exitCode = child.run(parentService);
                LOGINFO("End of the main process, shutting down");

                // teardown and close
                parentService.notifyOtherSideAndClose(); // notify the other side and close connection.
                thread.join();
            }
            catch (std::exception& ex)
            {
                LOGERROR("Child process exception: " << ex.what());
                if (exitCode == 0)
                {
                    exitCode = 1;
                }
                if (thread.joinable())
                {
                    thread.join();
                }
            }
            LOGDEBUG("Network process exiting");
            exit(exitCode);
        }
        else
        {
            io_service.notify_fork(boost::asio::io_context::fork_parent);
            boost::asio::signal_set signalSet{io_service, SIGCHLD};

            childChannel->justShutdownSocket();
            OtherSideApi childService(std::move(parentChannel));

            signalSet.async_wait(
                    [&](boost::system::error_code /*ec*/, int /*signo*/) {
                        parent.onOtherSideStop();
                    });

            config.applyParentSecurityPolicy();
            LOGINFO("Entering parent main process, pid:" << getpid());

            std::thread thread;
            try
            {
                thread = CommsContext::startThread(io_service);

                exitCode = parent.run(childService);

                childService.notifyOtherSideAndClose();
                LOGINFO("Waiting for network process to finish");

                int status;
                wait(&status);
                int childExitCode = WEXITSTATUS(status);
                if (childExitCode != 0)
                {
                    LOGINFO("Detected that child finished with status: " << status);
                    exitCode = childExitCode;
                }
                LOGINFO("Detected that parent process has finished");
                thread.join();
            }
            catch (std::exception& ex)
            {
                LOGERROR("Parent process exception: " << ex.what());
                if (exitCode == 0)
                {
                    exitCode = 1;
                }
                if (thread.joinable())
                {
                    thread.join();
                }
            }
            LOGDEBUG("Parent component is returning");
        }
        return exitCode;
    }

    /* Creates a ChildExEcutor that tunnel the messages received by the Parent into a
    *  MessageChannel which throw ChannelClosed when the Parent sends a stop message. 
    *  The functor does not need to capture that exception, it is handled by the childexecutor
    *  to teardown the childprocess. 
    *  It is usefull for cases where the child process does little more than reacting to messages 
    *   received by the parent. 
    */

    template<typename ParentExecutor, typename Config>
    int splitProcessesSimpleReactor(ParentExecutor parent, SimpleReactor child, Config config)
    {
        return splitProcesses(parent, ReactorAdapter{std::move(child), "child"}, config);
    }

    /* 
    Further simplification of splitProcessesSimpleReactor to provide the NullConfigurator as config. 
    It is likely to be used only in tests. But, given that it is only a template function, no code is 
    generated in production if not used, hence, no harm to have it here. 
    */
    template<typename ParentExecutor>
    int splitProcessesSimpleReactor(ParentExecutor parent, SimpleReactor child)
    {
        return splitProcessesSimpleReactor(parent, ReactorAdapter{std::move(child), "child"}, NullConfigurator{});
    }

    template<typename Config>
    int splitProcessesReactors(SimpleReactor parent, SimpleReactor child, Config config)
    {
        return splitProcesses(ReactorAdapter{std::move(parent), "parent"}, ReactorAdapter{std::move(child), "child"},
                              config);
    }

    /* Further simplification of splitProcessesReactors to pass NullConfigurator as config. 
       Again, likely to be used only in tests. But no harm to describe here as it will not go into production code anyway. 
    */
    int splitProcessesReactors(SimpleReactor parent, SimpleReactor child)
    {
        return splitProcessesReactors(std::move(parent), std::move(child), NullConfigurator{});
    }


} // namespace CommsComponent

