/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "ProductMetadata.h"
#include <cassert>
namespace SulDownloader
{
    const std::string &ProductMetadata::getLine() const
    {
        return m_line;
    }

    void ProductMetadata::setLine(const std::string &line)
    {
        m_line = line;
    }

    const std::string &ProductMetadata::getName() const
    {
        return m_name;
    }

    void ProductMetadata::setName(const std::string & name)
    {
        m_name = name;
    }

    void ProductMetadata::setTags(const std::vector<Tag>& tags)
    {
        m_tags = tags;
    }


    bool ProductMetadata::hasTag(const std::string & releaseTag) const
    {
        for (auto &tag : m_tags)
        {
            if (tag.tag.compare(releaseTag) == 0)
            {
                return true;
            }
        }
        return false;
    }

    void ProductMetadata::setVersion(const std::string &version)
    {
        m_version = version;
    }

    std::string ProductMetadata::getBaseVersion() const
    {
        auto pos = m_version.find('.');
        assert( pos != std::string::npos);
        return std::string( m_version.begin(), m_version.begin()+pos);
    }

    const std::string &ProductMetadata::getVersion() const
    {
        return m_version;
    }

    void ProductMetadata::setDefaultHomePath(const std::string &defaultHomeFolder)
    {
        m_defaultHomeFolder = defaultHomeFolder;
    }

    const std::string& ProductMetadata::getDefaultHomePath() const
    {
        return m_defaultHomeFolder;
    }


}