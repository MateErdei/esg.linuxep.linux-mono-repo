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
        explicit Subscriber(const std::string& socketAddress);
        ~Subscriber();
        void subscribeToEvents();
        void stop();
        void start();
    private:
        std::string m_socketPath; //"/opt/sophos-spl/plugins/eventjournaler/var/event.ipc";
        std::atomic<bool> m_running = false;
        std::unique_ptr<std::thread> m_runnerThread;

        bool waitFor(double timeToWaitInSeconds, double intervalInSeconds, std::function<bool()> conditionFunction);
    };
}

