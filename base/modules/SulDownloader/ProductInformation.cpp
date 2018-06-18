/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "ProductInformation.h"
#include <cassert>
namespace SulDownloader
{
    const std::string &ProductInformation::getLine() const
    {
        return m_line;
    }

    void ProductInformation::setLine(const std::string &line)
    {
        m_line = line;
    }

    const std::string &ProductInformation::getName() const
    {
        return m_name;
    }

    void ProductInformation::setName(const std::string & name)
    {
        m_name = name;
    }

    void ProductInformation::setTags(std::vector<Tag> tags)
    {
        m_tags = tags;
    }


    bool ProductInformation::hasTag(const std::string & releaseTag) const
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

    void ProductInformation::setVersion(const std::string &version)
    {
        m_version = version;
    }

    std::string ProductInformation::getBaseVersion() const
    {
        auto pos = m_version.find('.');
        assert( pos != std::string::npos);
        return std::string( m_version.begin(), m_version.begin()+pos);
    }

    const std::string &ProductInformation::getVersion() const
    {
        return m_version;
    }

    void ProductInformation::setDefaultHomePath(const std::string &defaultHomeFolder)
    {
        m_defaultHomeFolder = defaultHomeFolder;
    }

    std::string ProductInformation::getDefaultHomePath() const
    {
        return m_defaultHomeFolder;
    }


}