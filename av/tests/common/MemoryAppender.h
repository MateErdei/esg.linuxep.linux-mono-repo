/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "LogInitializedTests.h"

#include "datatypes/Print.h"

namespace
{
    using EventVector = std::vector<std::string>;

    class MemoryAppender : public log4cplus::Appender
    {
    public:
        MemoryAppender() = default;
        ~MemoryAppender() override;

        bool contains(const std::string& expected) const;
        EventVector m_events;
        void close() override {}
        EventVector::size_type size() const { return m_events.size(); }

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

        [[nodiscard]] bool appenderContains(const std::string& expected) const
        {
            assert(m_memoryAppender != nullptr);
            return m_memoryAppender->contains(expected);
        }

        [[nodiscard]] EventVector::size_type appenderSize() const
        {
            assert(m_memoryAppender != nullptr);
            return m_memoryAppender->size();
        }

        [[maybe_unused]] void dumpLog() const;

        bool waitForLog(const std::string& expected, int wait_time_micro_seconds=5000) const // NOLINT(modernize-use-nodiscard)
        {
            assert(m_memoryAppender != nullptr);
            struct timespec req{.tv_sec=0, .tv_nsec=10000};
            int count = wait_time_micro_seconds / 10;
            while (count > 0)
            {
                count--;
                if (appenderContains(expected))
                {
                    return true;
                }
                nanosleep(&req, nullptr);
            }
            return false;
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

    void MemoryAppenderUsingTests::dumpLog() const
    {
        PRINT("Memory appender contains " << appenderSize() << " items");
        for (const auto& item : m_memoryAppender->m_events)
        {
            PRINT("ITEM: " << item);
        }
    }

    template<const char* loggerInstanceName>
    class MemoryAppenderUsingTestsTemplate : public MemoryAppenderUsingTests
    {
    public:
        MemoryAppenderUsingTestsTemplate()
            : MemoryAppenderUsingTests(loggerInstanceName)
        {}
    };

    class UsingMemoryAppender
    {
    private:
        MemoryAppenderUsingTests& m_testClass;
    public:
        explicit UsingMemoryAppender(MemoryAppenderUsingTests& testClass)
            : m_testClass(testClass)
        {
            m_testClass.setupMemoryAppender();
        }
        ~UsingMemoryAppender()
        {
            m_testClass.clearMemoryAppender();
        }
    };
}