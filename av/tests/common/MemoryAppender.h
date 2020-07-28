/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "LogInitializedTests.h"

namespace
{
    class MemoryAppender : public log4cplus::Appender
    {
    public:
        MemoryAppender() = default;
        ~MemoryAppender() override;

        bool contains(const std::string& expected) const;
        std::vector<std::string> m_events;
        void close() override {}
        std::vector<std::string>::size_type size() const { return m_events.size(); }

        void clear() noexcept
        {
            m_events.clear();
        }
    protected:
        void append(const log4cplus::spi::InternalLoggingEvent& event) override;

    };

    MemoryAppender::~MemoryAppender()
    {
        destructorImpl();
    }

    void MemoryAppender::append(const log4cplus::spi::InternalLoggingEvent& event)
    {
        std::stringstream logOutput;
        layout->formatAndAppend(logOutput, event);
        m_events.emplace_back(logOutput.str());
    }
    bool MemoryAppender::contains(const std::string& expected) const
    {
        for (const auto& e : m_events)
        {
            if (e.find(expected) != std::string::npos)
            {
                return true;
            }
        }
        return false;
    }


    class MemoryAppenderUsingTests : public LogInitializedTests
    {
    public:
        explicit MemoryAppenderUsingTests(std::string loggerInstanceName);
        std::string m_loggerInstanceName;
        MemoryAppender* m_memoryAppender = nullptr;
        log4cplus::SharedAppenderPtr m_sharedAppender;

        void setupMemoryAppender();
        void clearMemoryAppender();

        [[nodiscard]] log4cplus::Logger getLogger() const;

        [[nodiscard]] bool appender_contains(const std::string& expected) const
        {
            assert(m_memoryAppender != nullptr);
            return m_memoryAppender->contains(expected);
        }

        void wait_for_log(const std::string& expected)
        {
            assert(m_memoryAppender != nullptr);
            struct timespec req{.tv_sec=0, .tv_nsec=10000};
            int count = 50;
            while (count > 0)
            {
                count--;
                if (appender_contains(expected))
                {
                    return;
                }
                nanosleep(&req, nullptr);
            }
        }
    };

    MemoryAppenderUsingTests::MemoryAppenderUsingTests(std::string loggerInstanceName)
        : m_loggerInstanceName(std::move(loggerInstanceName))
    {}

    void MemoryAppenderUsingTests::setupMemoryAppender()
    {
        if (!m_sharedAppender)
        {
            log4cplus::Logger logger = getLogger();
            m_memoryAppender = new MemoryAppender; // Declared separately so that we can access MemoryAppender
            m_sharedAppender = log4cplus::SharedAppenderPtr(m_memoryAppender); // Donated pointer
            logger.addAppender(m_sharedAppender);
        }
    }

    void MemoryAppenderUsingTests::clearMemoryAppender()
    {
        if (m_sharedAppender)
        {
            log4cplus::Logger logger = getLogger();
            logger.removeAppender(m_sharedAppender);
            m_memoryAppender->clear();
            m_memoryAppender = nullptr;
            m_sharedAppender = log4cplus::SharedAppenderPtr();
        }
    }

    log4cplus::Logger MemoryAppenderUsingTests::getLogger() const
    {
        return Common::Logging::getInstance(m_loggerInstanceName);
    }
}