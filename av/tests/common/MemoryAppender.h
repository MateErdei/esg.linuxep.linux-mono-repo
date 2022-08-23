/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "LogInitializedTests.h"

#include "datatypes/Print.h"

#include <chrono>
#include <numeric>
#include <thread>

namespace
{
    using namespace std::chrono_literals;

    // use std::list to avoid the cost
    using EventCollection = std::vector<std::string>;

    class MemoryAppender : public log4cplus::Appender
    {
    public:
        MemoryAppender() = default;
        ~MemoryAppender() override;

        int count(const std::string& expected) const;
        bool contains(const std::string& expected) const;
        bool containsCount(const std::string& expected, int count) const;
        void close() override {}
        EventCollection::size_type size() const;
        void clear() noexcept;

        EventCollection m_events;
        mutable std::mutex m_events_mutex;

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
        const std::lock_guard lock(m_events_mutex);
        m_events.emplace_back(logOutput.str());
    }

    bool MemoryAppender::contains(const std::string& expected) const
    {
        const std::lock_guard lock(m_events_mutex);
        auto contains_expected = [&](const std::string& e) { return e.find(expected) != std::string::npos; };
        return std::any_of(m_events.begin(), m_events.end(), contains_expected);
    }

    int countSubstring(const std::string& str, const std::string& sub)
    {
        if (sub.length() == 0)
            return 0;
        int count = 0;
        for (size_t offset = str.find(sub); offset != std::string::npos; offset = str.find(sub, offset + sub.length()))
        {
            ++count;
        }
        return count;
    }

    int MemoryAppender::count(const std::string& expected) const
    {
        const std::lock_guard lock(m_events_mutex);
        auto count_expected = [&](int c, const std::string& e) { return c + countSubstring(e, expected); };
        auto actualCount = std::accumulate(m_events.begin(), m_events.end(), 0, count_expected);
        return actualCount;
    }

    bool MemoryAppender::containsCount(const std::string& expected, int expectedCount) const
    {
        return expectedCount == count(expected);
    }

    EventCollection::size_type MemoryAppender::size() const
    {
        const std::lock_guard lock(m_events_mutex);
        return m_events.size();
    }

    void MemoryAppender::clear() noexcept
    {
        const std::lock_guard lock(m_events_mutex);
        m_events.clear();
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

        [[nodiscard]] int appenderCount(const std::string& expected) const
        {
            assert(m_memoryAppender != nullptr);
            return m_memoryAppender->count(expected);
        }

        [[nodiscard]] bool appenderContains(const std::string& expected) const
        {
            assert(m_memoryAppender != nullptr);
            return m_memoryAppender->contains(expected);
        }

        [[nodiscard]] bool appenderContains(const std::string& expected, int expectedCount) const
        {
            assert(m_memoryAppender != nullptr);
            return m_memoryAppender->containsCount(expected, expectedCount);
        }

        [[nodiscard]] bool appenderContainsCount(const std::string& expected, int expectedCount) const
        {
            assert(m_memoryAppender != nullptr);
            return m_memoryAppender->containsCount(expected, expectedCount);
        }

        [[nodiscard]] EventCollection::size_type appenderSize() const
        {
            assert(m_memoryAppender != nullptr);
            return m_memoryAppender->size();
        }

        [[maybe_unused]] void dumpLog() const;

        using clock = std::chrono::steady_clock;
        [[maybe_unused]] bool waitForLog(const std::string& expected, clock::duration wait_time = 250ms) const; // NOLINT(modernize-use-nodiscard)
        [[maybe_unused]] bool waitForLogMultiple(const std::string& expected, const int& count, clock::duration wait_time = 100ms) const; // NOLINT(modernize-use-nodiscard)
    };

    MemoryAppenderUsingTests::MemoryAppenderUsingTests(std::string loggerInstanceName) :
        m_loggerInstanceName(std::move(loggerInstanceName))
    {
    }

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

        const std::lock_guard lock(m_memoryAppender->m_events_mutex);
        for (const auto& item : m_memoryAppender->m_events)
        {
            PRINT("ITEM: " << item);
        }
    }

    bool MemoryAppenderUsingTests::waitForLog(const std::string& expected, clock::duration wait_time) const
    {
        assert(m_memoryAppender != nullptr);
        auto deadline = clock::now() + wait_time;
        do
        {
            if (appenderContains(expected))
            {
                return true;
            }
            std::this_thread::sleep_for(10ms);
        } while (clock::now() < deadline);
        return false;
    }

    bool MemoryAppenderUsingTests::waitForLogMultiple(const std::string& expected, const int& count, clock::duration wait_time) const
    {
        assert(m_memoryAppender != nullptr);
        auto deadline = clock::now() + wait_time;
        do
        {
            if (appenderContainsCount(expected, count))
            {
                return true;
            }
            std::this_thread::sleep_for(10ms);
        } while (clock::now() < deadline);
        return false;
    }

    template<const char* loggerInstanceName>
    class MemoryAppenderUsingTestsTemplate : public MemoryAppenderUsingTests
    {
    public:
        MemoryAppenderUsingTestsTemplate() : MemoryAppenderUsingTests(loggerInstanceName) {}
    };

    class UsingMemoryAppender
    {
    private:
        MemoryAppenderUsingTests& m_testClass;

    public:
        explicit UsingMemoryAppender(MemoryAppenderUsingTests& testClass) : m_testClass(testClass)
        {
            m_testClass.setupMemoryAppender();
        }
        ~UsingMemoryAppender()
        {
            m_testClass.teardownMemoryAppender();
        }
    };
} // namespace