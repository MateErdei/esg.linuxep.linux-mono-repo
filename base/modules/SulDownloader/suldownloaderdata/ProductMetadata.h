/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Tag.h"

#include <string>
#include <vector>

namespace SulDownloader
{
    namespace suldownloaderdata
    {
        /**
         * Product meatadata from the warehouse.
         */
        class ProductMetadata
        {
        public:
            const std::string& getLine() const;

            void setLine(const std::string& line);

            const std::string& getName() const;

            void setName(const std::string& name);

            void setTags(const std::vector<Tag>& tags);

            bool hasTag(const std::string& releaseTag) const;
            const std::vector<Tag> tags() const;

            std::string getBaseVersion() const;

            const std::string& getVersion() const;

            void setVersion(const std::string& version);
            void setBaseVersion( const std::string & baseVersion);

            void setDefaultHomePath(const std::string& defaultHomeFolder);
            void setFeatures(const std::vector<std::string> & features);
            const std::vector<std::string>& getFeatures() const;

            const std::string& getDefaultHomePath() const;

        private:
            std::vector<Tag> m_tags;
            std::string m_line;
            std::string m_name;
            std::string m_version;
            std::string m_baseVersion;
            std::string m_defaultHomeFolder;
            std::vector<std::string> m_features;
        };
    } // namespace suldownloaderdata
} // namespace SulDownloader
