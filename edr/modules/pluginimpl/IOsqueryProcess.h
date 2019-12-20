/******************************************************************************************************

Copyright 2019-2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <exception>
#include <memory>

namespace Plugin
{
    class IOsqueryCrashed : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    class IOsqueryCannotBeExecuted : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    class IOsqueryProcess
    {
    public:
        virtual ~IOsqueryProcess() = default;
        virtual void keepOsqueryRunning() = 0;
        virtual void requestStop() = 0;
    };
    using IOsqueryProcessPtr = std::unique_ptr<IOsqueryProcess>;
    extern IOsqueryProcessPtr createOsqueryProcess();

} // namespace Plugin