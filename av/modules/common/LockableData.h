// Copyright 2022-2023 Sophos Limited. All rights reserved.

// from https://devblogs.microsoft.com/oldnewthing/20221005-00/?p=107248

#pragma once

#include <memory>
#include <mutex>

namespace common
{
    template<typename>
    struct LockableData;
}

namespace std
{
    template<typename Data>
    struct default_delete<common::LockableData<Data>>
    {
        void operator()(common::LockableData<Data>* p)
            const noexcept { p->m_mutex.unlock(); }
    };
}

namespace common
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
    struct LockableData
    {
        LockableData() = default;
        explicit LockableData(Data val)
            : m_data(std::move(val))
        {
        }

        LockedData<LockableData> lock() { return LockedData(this); }
        const Data& ref() const noexcept
        {
            return m_data;
        }
    private:
        Data& ref() noexcept
        {
            return m_data;
        }

        friend struct LockedData<LockableData>;
        friend struct std::default_delete<LockableData>;

        std::mutex m_mutex;
        Data m_data;
    };
}