// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include "Reloader.h"

#include "ThreatDetectorException.h"

void sspl::sophosthreatdetectorimpl::Reloader::update()
{
    if (!m_scannerFactory)
    {
        throw ThreatDetectorException(LOCATION, "Failed to update threat scanner: No scanner factory available");
    }
    else if (!m_scannerFactory->update())
    {
        // We throw here because this means SUSI was already initialised, but failed to update.
        // Either this is a persistent problem - which is handled because the 'pending' update in the init code doesn't exit
        // Or it's a transient problem - in which case a restart will recover SUSI
        throw SusiUpdateFailedException(LOCATION, "Failed to update threat scanner");
    }
}

void sspl::sophosthreatdetectorimpl::Reloader::reload()
{
    if (!m_scannerFactory)
    {
        throw ThreatDetectorException(LOCATION, "Failed to reload threat scanner: No scanner factory available");
    }
    else if (!m_scannerFactory->reload())
    {
        throw ThreatDetectorException(LOCATION, "Failed to reload threat scanner");
    }
}

bool sspl::sophosthreatdetectorimpl::Reloader::updateSusiConfig()
{
    return m_scannerFactory->updateSusiConfig();
}