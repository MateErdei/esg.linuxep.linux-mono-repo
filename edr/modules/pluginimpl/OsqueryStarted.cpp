/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <boost/assert.hpp>
#include "OsqueryStarted.h"

namespace Plugin{

    void OsqueryStarted::announce_started()
    {
        assert(!m_announced_already);
        m_announced_already = true;
        m_promise.set_value();
    }

    void OsqueryStarted::wait_started()
    {
        assert(!m_waited_already);
        m_waited_already = true;
        m_promise.get_future().wait();
    }
}
