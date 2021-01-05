/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <memory>

namespace unixsocket
{
    class IReloadable
    {
    public:
        virtual void reload() = 0;
        virtual ~IReloadable() = default;
    };
    using IReloadablePtr = std::shared_ptr<IReloadable>;

}