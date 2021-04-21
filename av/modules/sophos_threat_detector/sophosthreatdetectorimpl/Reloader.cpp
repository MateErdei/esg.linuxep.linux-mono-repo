/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Reloader.h"

void sspl::sophosthreatdetectorimpl::Reloader::reload()
{
    if (!m_scannerFactory)
    {
        throw std::runtime_error("Failed to update threat scanner: No scanner factory available");
    }
    else if (!m_scannerFactory->update())
    {
        throw std::runtime_error("Failed to update threat scanner");
    }
}
