// Copyright 2022, Sophos Limited.  All rights reserved.

// from https://devblogs.microsoft.com/oldnewthing/20221005-00/?p=107248

#pragma once

#include <memory>
#include <mutex>

namespace Common::Threads
{
    template<typename>
    struct LockableData;
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

    private:
        std::unique_ptr<Lockable> l;
    };

    template<typename Data>
    struct LockableData
    {
        LockedData<LockableData> lock() { return LockedData(this); }

    private:
        friend struct LockedData<LockableData>;
        friend struct std::default_delete<LockableData>;

        std::mutex m_mutex;
        Data m_data;
    };
}