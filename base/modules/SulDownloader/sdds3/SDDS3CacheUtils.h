// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "Sdds3Wrapper.h"

#include "Common/XmlUtilities/AttributesMap.h"
#include <set>

namespace SulDownloader
{
    class SDDS3CacheUtils
    {
        public:
            bool checkCache(const std::set<std::string> &suites, const std::set<std::string> &releaseGroups);

        private:
            void clearEntireCache();
            bool isCacheUpToDate(const std::string &baseSuitePath, const std::set<std::string> &releaseGroups);
        std::vector<std::pair<std::string,std::string>> getSupplementRefsFromBaseSuite(const std::string &baseSuitePath);
            Common::XmlUtilities::AttributesMap  getXMLFromDatFile(const std::string &filepath);
    };


}