// Copyright 2021-2022, Sophos Limited.  All rights reserved.

#pragma once

#include <memory>

namespace sspl::sophosthreatdetectorimpl
{
    class IReloadable
    {
    public:
        virtual void update() = 0;
        virtual void reload() = 0;
        virtual ~IReloadable() = default;
    };
    using IReloadablePtr = std::shared_ptr<IReloadable>;

}