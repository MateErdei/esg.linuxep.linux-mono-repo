/******************************************************************************************************

Copyright 2019-2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <future>

namespace Plugin{
    class OsqueryStarted {
        std::promise<void> m_promise;
        bool m_announced_already = false;
        bool m_waited_already = false;

    public:
//        these methods can only be used once
        void announce_started();
        void wait_started();
    };

}
