/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Telemetry.h"

namespace UpdateScheduler
{

    Telemetry &Telemetry::instance()
    {
        static Telemetry telemetry;
        return telemetry;
    }

    std::string Telemetry::getJson() const
    {
    }

    void Telemetry::clear()
    {

    }
}