/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>
#include <vector>

#include "Tag.h"

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

            std::string getBaseVersion() const;

            const std::string& getVersion() const;

            void setVersion(const std::string& version);

            void setDefaultHomePath(const std::string& defaultHomeFolder);

            const std::string& getDefaultHomePath() const;

        private:
            std::vector<Tag> m_tags;
            std::string m_line;
            std::string m_name;
            std::string m_version;
            std::string m_defaultHomeFolder;
        };
    }
}

