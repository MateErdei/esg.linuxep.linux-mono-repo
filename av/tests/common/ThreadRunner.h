#include "modules/pluginimpl/Logger.h"

namespace
{
    class ThreadRunner
    {
    public:
        explicit ThreadRunner(Common::Threads::AbstractThread &thread, std::string name)
                : m_thread(thread), m_name(std::move(name))
        {
            LOGINFO("Starting " << m_name);
            m_thread.start();
        }

        void killThreads()
        {
            LOGINFO("Stopping " << m_name);
            m_thread.requestStop();
            LOGINFO("Joining " << m_name);
            m_thread.join();
        }

        Common::Threads::AbstractThread &m_thread;
        std::string m_name;
    };
}