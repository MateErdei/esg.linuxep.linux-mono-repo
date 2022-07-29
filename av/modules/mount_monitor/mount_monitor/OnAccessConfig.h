/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

namespace mount_monitor::mount_monitor
{
        class OnAccessConfig
        {
        public:
            bool m_scanHardDisc = true;
            bool m_scanOptical = true;
            bool m_scanNetwork = true;
            bool m_scanRemovable = true;
        };
}
