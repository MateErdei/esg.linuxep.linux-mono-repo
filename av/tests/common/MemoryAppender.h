/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "LogInitializedTests.h"

#include "datatypes/Print.h"

#include <chrono>
#include <thread>


namespace
{
    using namespace std::chrono_literals;

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
        auto contains_expected = [&](const std::string& e){ return e.find(expected) != std::string::npos; };
        return std::any_of(m_events.begin(), m_events.end(), contains_expected);
    }


    class MemoryAppenderUsingTests : public LogInitializedTests
    {
    public:
        explicit MemoryAppenderUsingTests(std::string loggerInstanceName);
        std::string m_loggerInstanceName;
        MemoryAppender* m_memoryAppender = nullptr;
        log4cplus::SharedAppenderPtr m_sharedAppender;

        void setupMemoryAppender();
        void teardownMemoryAppender();
        void clearMemoryAppender() const;

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

        typedef std::chrono::steady_clock clock;

        bool waitForLog(const std::string& expected, clock::duration wait_time = 100ms) const; // NOLINT(modernize-use-nodiscard)

        [[maybe_unused]] bool waitForLog(const std::string& expected, int wait_time_micro_seconds) const // NOLINT(modernize-use-nodiscard)
        {
            return waitForLog(expected, std::chrono::microseconds(wait_time_micro_seconds));
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

    void MemoryAppenderUsingTests::teardownMemoryAppender()
    {
        if (m_sharedAppender)
        {
            log4cplus::Logger logger = getLogger();
            logger.removeAppender(m_sharedAppender);
            m_memoryAppender->clear();
            m_memoryAppender = nullptr;
            m_sharedAppender = log4cplus::SharedAppenderPtr(); // deletes m_memoryAppender
        }
    }

    [[maybe_unused]] void MemoryAppenderUsingTests::clearMemoryAppender() const
    {
        assert(m_memoryAppender != nullptr);
        m_memoryAppender->clear();
    }

    log4cplus::Logger MemoryAppenderUsingTests::getLogger() const
    {
        return Common::Logging::getInstance(m_loggerInstanceName);
    }

    [[maybe_unused]] void MemoryAppenderUsingTests::dumpLog() const
    {
        assert(m_memoryAppender != nullptr);
        PRINT("Memory appender contains " << appenderSize() << " items");
        for (const auto& item : m_memoryAppender->m_events)
        {
            PRINT("ITEM: " << item);
        }
    }

    bool MemoryAppenderUsingTests::waitForLog(const std::string& expected, clock::duration wait_time) const // NOLINT(modernize-use-nodiscard)
    {
        assert(m_memoryAppender != nullptr);
        auto deadline = clock::now() + wait_time;
        do
        {
            if (appenderContains(expected))
            {
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        } while (clock::now() < deadline);
        return false;
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
            m_testClass.teardownMemoryAppender();
        }
    };
}