//------------------------------------------------------------------------------
// Copyright 2015-2020 Sophos Limited. All rights reserved.
//
// Sophos is a registered trademark of Sophos Limited and Sophos Group. All
// other product and company names mentioned are trademarks or registered
// trademarks of their respective owners.
//------------------------------------------------------------------------------

#pragma once

//#pragma warning(push)
#include "UpdateSchedulerImpl/stateMachines/BoostIgnoreWarnings.h"
#include <boost/noncopyable.hpp>
//#pragma warning(pop)

//#include <experimental/filesystem>
#include <string>
#include <vector>

struct ISULSession;

class ICacheEvaluator : public boost::noncopyable
{
public:
    struct CacheLocation
    {
        CacheLocation(std::string const &cacheHostname, int cachePriority, std::string const &cacheEndpointId)
            : hostname(cacheHostname)
            , priority(cachePriority)
            , endpointId(cacheEndpointId)
            , secure(true)
        {
        }

        std::string hostname;
        int priority;
        std::string endpointId;
        std::string dciFilename;
        bool secure;

        bool operator<(const CacheLocation& that) const
        {
            // Sort Update Caches by their priority first, then by address, then by port.
            if (priority != that.priority)
            {
                return priority < that.priority;
            }

            return hostname < that.hostname;
        }

        bool operator==(const CacheLocation& that) const
        {
            return (priority == that.priority)
                && (hostname == that.hostname)
                && (endpointId == that.endpointId);
        }
    };

    // For each cache, attempt to download *one* of these resources relative to the cache host.
    virtual void Evaluate(const std::vector<std::string>& resources) = 0;
    virtual bool UseCache() const = 0;
    virtual const CacheLocation& Cache() const = 0;

 //   virtual std::experimental::filesystem::path SslCertificateStore() const = 0;

    virtual ~ICacheEvaluator() {}

protected:
    ICacheEvaluator() {}
};
