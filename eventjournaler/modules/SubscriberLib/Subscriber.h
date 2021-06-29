/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <atomic>
#include <functional>
#include <string>
#include <thread>

namespace SubscriberLib
{
    class Subscriber
    {
    public:
        Subscriber(const std::string& socketAddress, int readLoopTimeoutMilliSeconds = 5000);
        ~Subscriber();
        void subscribeToEvents();
        void stop();
        void start();
        void reset();
        bool getRunningStatus();
    private:
        std::string m_socketPath ;
        std::atomic<bool> m_running = false;
        int m_readLoopTimeoutMilliSeconds;
        std::unique_ptr<std::thread> m_runnerThread;
    };
}

