// Copyright 2022, Sophos Limited.  All rights reserved.

#include "Logger.h"
#include "OnAccessTelemetryUtility.h"

#include <cassert>


using namespace sophos_on_access_process::onaccessimpl::onaccesstelemetry;

OnAccessTelemetryUtility::OnAccessTelemetryUtility()
{
    m_eventsDropped.store(0);
    m_eventsReceived.store(0);
    m_scanErrors.store(0);
    m_scansRequested.store(0);
}

OnAccessTelemetryUtility::TelemetryEntry OnAccessTelemetryUtility::getTelemetry()
{
    TelemetryEntry telemetryToSend{};

    telemetryToSend.m_percentageEventsDropped = calculatePerEventsDropped();
    telemetryToSend.m_percentageScanErrors = calculatePerScanErrors();

    return telemetryToSend;
}

float OnAccessTelemetryUtility::calculatePerEventsDropped()
{
    uint64_t eventsDropped = m_eventsDropped.exchange(0);
    uint64_t eventsReceived = m_eventsReceived.exchange(0);
    m_eventsAtLimit.store(false);

    return calculatePercentage(eventsReceived, eventsDropped);
}

float OnAccessTelemetryUtility::calculatePerScanErrors()
{
    uint64_t scanErrors = m_scanErrors.exchange(0);
    uint64_t scansRequested = m_scansRequested.exchange(0);
    m_scansAtLimit.store(false);

    return calculatePercentage(scansRequested, scanErrors);
}

float OnAccessTelemetryUtility::calculatePercentage(unsigned long total, unsigned int problems)
{
    assert(problems <= total);

    if (problems == 0 || total == 0)
    {
        return 0;
    }
    else if (problems == total)
    {
        return 100;
    }

    double doubleproblems = static_cast<double>(problems);
    return (doubleproblems / total) * 100;
}

void OnAccessTelemetryUtility::incrementEventReceived(bool dropped)
{
    if (m_eventsReceived.load() == std::numeric_limits<unsigned long>::max() ||
        m_eventsDropped.load() == std::numeric_limits<unsigned int>::max())
    {
        if (!m_eventsAtLimit.load())
        {
            LOGWARN("A Telemetry Value for Events at limit");
            m_eventsAtLimit.store(true);
        }
        return;
    }

    m_eventsReceived++;
    if (dropped)
    {
        m_eventsDropped++;
    }
}

void OnAccessTelemetryUtility::incrementFilesScanned(bool error)
{
    if (m_scansRequested.load() == std::numeric_limits<unsigned long>::max() ||
        m_scanErrors.load() == std::numeric_limits<unsigned int>::max())
    {
        if (!m_scansAtLimit.load())
        {
            LOGWARN("A Telemetry Value for Scans at limit");
            m_scansAtLimit.store(true);
        }
        return;
    }

    m_scansRequested++;
    if (error)
    {
        m_scanErrors++;
    }
}