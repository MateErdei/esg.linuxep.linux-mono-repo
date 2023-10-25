// Copyright 2023 Sophos All rights reserved.

#pragma once

#include <mutex>

namespace Common::Threads
{
    class LockableBool
    {
    private:
        bool value_;
        std::mutex lock_;
    public:
        explicit LockableBool(bool v)
            : value_(v)
        {
        }

        bool load()
        {
            std::lock_guard locked{lock_};
            return value_;
        }

        void store(bool v)
        {
            std::lock_guard locked{lock_};
            value_ = v;
        }
    };
}
