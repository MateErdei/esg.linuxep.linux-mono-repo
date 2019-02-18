///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
#pragma once

#include <memory>
#include <string>

namespace Common
{
    namespace ZeroMQWrapper
    {
        class IProxy
        {
        public:
            virtual ~IProxy() = default;
            virtual void start() = 0;
            virtual void stop() = 0;
        };

        using IProxyPtr = std::unique_ptr<IProxy>;
    } // namespace ZeroMQWrapper
} // namespace Common
