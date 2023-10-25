// Copyright 2023 Sophos Limited. All rights reserved.


#include "PolicyHelper.h"
#include "PolicyParseException.h"

#include "Common/OSUtilities/IIPUtils.h"

#include <string>

namespace Common::Policy {
    template<typename IpCollection>
    std::string ipCollectionToString(const IpCollection &ipCollection) {
        if (ipCollection.empty()) {
            return std::string{};
        }
        std::stringstream s;
        bool first = true;
        for (auto &ip: ipCollection) {
            if (first) {
                s << "{";
            } else {
                s << ", ";
            }
            s << ip.stringAddress();
            first = false;
        }
        s << "}";
        return s.str();
    }

    std::string ipToString(const Common::OSUtilities::IPs &ips) {
        std::stringstream s;
        bool hasip4 = false;
        if (!ips.ip4collection.empty()) {
            s << ipCollectionToString(ips.ip4collection);
            hasip4 = true;
        }

        if (!ips.ip6collection.empty()) {
            if (hasip4) {
                s << "\n";
            }
            s << ipCollectionToString(ips.ip6collection);
        }
        return s.str();
    }


    std::string reportToString(const Common::OSUtilities::SortServersReport &report) {
        std::stringstream s;
        s << "Local ips: \n"
          << ipToString(report.localIps) << "\n"
          << "Servers: \n";
        for (auto &server: report.servers) {
            s << server.uri << '\n';
            if (!server.error.empty()) {
                s << server.error << '\n';
            } else {
                s << ipToString(server.ips) << '\n';
            }
            s << "distance associated: ";
            if (server.MaxDistance == server.associatedMinDistance) {
                s << "Max\n";
            } else {
                s << server.associatedMinDistance << " bits\n";
            }
        }
        return s.str();
    }
}
