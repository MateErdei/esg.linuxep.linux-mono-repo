// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include <chrono>
#include <ctime>
#include <string>
#include <vector>

#ifndef TEST_PUBLIC
# define TEST_PUBLIC private
#endif

namespace Plugin
{
    /**
     * Wait for initial policies to arrive, and log if they aren't delivered
     */
    class PolicyWaiter
    {
    public:
        using seconds_t = std::chrono::seconds;
        using policy_list_t = std::vector<std::string>;
        static constexpr seconds_t DEFAULT_WARNING_TIMEOUT{3600};
        PolicyWaiter(policy_list_t expectedPolicies, seconds_t infoTimeout, seconds_t warningTimeout=DEFAULT_WARNING_TIMEOUT);

        PolicyWaiter(const PolicyWaiter&) = delete;
        PolicyWaiter(PolicyWaiter&&) = delete;

        /**
         * Get the timeout before we should be alerted
         * @return
         */
        [[nodiscard]] seconds_t timeout() const;

        /**
         * Notify the Policy Waiter that we've got a policy
         * @param appId
         */
        void gotPolicy(const std::string& appId);

        /**
         * Check if we should log something due to not receiving policies
         */
        void checkTimeout();

    TEST_PUBLIC:
        PolicyWaiter(time_t now, policy_list_t expectedPolicies, seconds_t infoTimeout, seconds_t warningTimeout=DEFAULT_WARNING_TIMEOUT);

        /**
         * Get the timeout before we should be alerted
         * @return
         */
        [[nodiscard]] seconds_t timeout(time_t now) const;


        /**
         * Check if we should log something due to not receiving policies
         */
        void checkTimeout(time_t now);

    private:
        /**
         * List of expected policies we haven't received yet
         */
        policy_list_t m_pendingPolicies;

        time_t m_start;
        seconds_t m_infoTimeout;
        seconds_t m_warningTimeout;
        bool m_infoLogged = false;
        bool m_warningLogged = false;

    };
}
