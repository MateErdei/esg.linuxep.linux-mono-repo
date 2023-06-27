// Copyright 2022-2023 Sophos Limited. All rights reserved.

// from https://devblogs.microsoft.com/oldnewthing/20221005-00/?p=107248

#pragma once

#include <memory>
#include <mutex>

namespace Common::Threads
{
    template<typename>
    class LockableData;
}

namespace std
{
    template<typename Data>
    struct default_delete<Common::Threads::LockableData<Data>>
    {
        void operator()(Common::Threads::LockableData<Data>* p)
            const noexcept { p->m_mutex.unlock(); }
    };
}

namespace Common::Threads
{
    template<typename Lockable>
    struct [[nodiscard]] LockedData
    {
        explicit LockedData(Lockable* l = nullptr) : l(l)
        { if (l) l->m_mutex.lock(); }

        auto operator->() const noexcept
        { return std::addressof(l->m_data); }

        auto& operator*() noexcept
        { return l->ref(); }

        const auto& operator*() const noexcept
        { return l->ref(); }

    private:
        std::unique_ptr<Lockable> l;
    };

    template<typename Data>
    class LockableData
    {
    public:
        using lock_t = LockedData<LockableData>;
        LockableData() = default;
        explicit LockableData(Data initialValue)
            : m_data(std::move(initialValue))
        {}
        lock_t lock() { return LockedData(this); }

    private:
        friend struct LockedData<LockableData>;
        friend struct std::default_delete<LockableData>;

        const Data& ref() const noexcept
        {
            return m_data;
        }

        Data& ref() noexcept
        {
            return m_data;
        }

        std::mutex m_mutex;
        Data m_data;
    };
}