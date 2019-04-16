/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

namespace Common
{
    namespace Telemetry
    {
        class ITelemetry
        {
        public:
            virtual ~ITelemetry() = default;

            virtual void add() = 0;
            virtual void update() = 0;
        };
    } // namespace TaskQueue
} // namespace Common
