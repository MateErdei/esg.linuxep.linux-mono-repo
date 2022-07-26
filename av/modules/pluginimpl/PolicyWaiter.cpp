// Copyright 2022, Sophos Limited.  All rights reserved.

#include "PolicyWaiter.h"

#include "Logger.h"

#include <cassert>

using namespace Plugin;

PolicyWaiter::PolicyWaiter(policy_list_t expectedPolicies, seconds_t infoTimeout, seconds_t warningTimeout)
    : PolicyWaiter(::time(nullptr), std::move(expectedPolicies), infoTimeout, warningTimeout)
{}

PolicyWaiter::PolicyWaiter(time_t now, policy_list_t expectedPolicies, std::chrono::seconds infoTimeout, std::chrono::seconds warningTimeout)
    : m_pendingPolicies{std::move(expectedPolicies)}
    , m_start(now)
    , m_infoTimeout(infoTimeout)
    , m_warningTimeout(warningTimeout)
{}

void PolicyWaiter::gotPolicy(const std::string& appId)
{
    auto it = std::find(m_pendingPolicies.begin(), m_pendingPolicies.end(), appId);
    if (it != m_pendingPolicies.end())
    {
        m_pendingPolicies.erase(it);
    }
}

std::chrono::seconds Plugin::PolicyWaiter::timeout() const
{
    return timeout(::time(nullptr));
}

std::chrono::seconds PolicyWaiter::timeout(time_t now) const
{
    /**
     * Can't be MAX int since we want to add to a now value in TaskQueue
     */
    static constexpr std::chrono::seconds MAX_TIMEOUT{9999999};
    static constexpr std::chrono::seconds ZERO{0};

    if (m_pendingPolicies.empty())
    {
        // Received all policies so don't wake up again
        return MAX_TIMEOUT;
    }

    std::chrono::seconds delay{now - m_start};
    if (!m_infoLogged)
    {
        // Not received policies yet, and not logged info message
        // So return delay before infoTimeout
        return std::max(m_infoTimeout - delay, ZERO); // But don't go negative
    }
    else if (!m_warningLogged)
    {
        // Not received policies yet, and not logged warning message
        // So return delay before warningTimeout
        return std::max(m_warningTimeout - delay, ZERO); // But don't go negative
    }
    else
    {
        // Done both logging so don't wake up again
        return MAX_TIMEOUT;
    }
}

void PolicyWaiter::checkTimeout()
{
    return checkTimeout(::time(nullptr));
}

void PolicyWaiter::checkTimeout(time_t now)
{
    if (m_warningLogged)
    {
        return; // don't both checking any more
    }

    if (m_pendingPolicies.empty())
    {
        return; // Don't log anything - we've got the policies!
    }

    std::chrono::seconds delay{now - m_start};
    if (delay > m_warningTimeout)
    {
        assert(!m_warningLogged);
        for (const auto& policy : m_pendingPolicies)
        {
            LOGWARN("Failed to get " << policy << " policy");
        }
        m_warningLogged = true;
        m_infoLogged = true; // avoid double logging
        return;
    }
    if (delay > m_infoTimeout && !m_infoLogged)
    {
        for (const auto& policy : m_pendingPolicies)
        {
            LOGINFO("Failed to get " << policy << " policy at startup");
        }
        m_infoLogged = true;
        return;
    }
}
