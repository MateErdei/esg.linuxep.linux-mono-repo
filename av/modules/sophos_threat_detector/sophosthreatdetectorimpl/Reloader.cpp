/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Reloader.h"

#include "Logger.h"

void sspl::sophosthreatdetectorimpl::Reloader::update()
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

void sspl::sophosthreatdetectorimpl::Reloader::reload()
{
    if (!m_scannerFactory)
    {
        throw std::runtime_error("Failed to reload threat scanner: No scanner factory available");
    }
    else if (!m_scannerFactory->reload())
    {
        throw std::runtime_error("Failed to reload threat scanner");
    }
}

bool sspl::sophosthreatdetectorimpl::Reloader::updateSusiConfig()
{
    return m_scannerFactory->updateSusiConfig();
}