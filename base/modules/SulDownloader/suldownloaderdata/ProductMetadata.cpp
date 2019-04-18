/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "ProductMetadata.h"

#include <cassert>

using namespace SulDownloader::suldownloaderdata;

const std::string& ProductMetadata::getLine() const
{
    return m_line;
}

void ProductMetadata::setLine(const std::string& line)
{
    m_line = line;
}

const std::string& ProductMetadata::getName() const
{
    return m_name;
}

void ProductMetadata::setName(const std::string& name)
{
    m_name = name;
}

void ProductMetadata::setTags(const std::vector<Tag>& tags)
{
    m_tags = tags;
}

bool ProductMetadata::hasTag(const std::string& releaseTag) const
{
    for (auto& tag : m_tags)
    {
        if (tag.tag == releaseTag)
        {
            return true;
        }
    }
    return false;
}

void ProductMetadata::setVersion(const std::string& version)
{
    m_version = version;
}

std::string ProductMetadata::getBaseVersion() const
{
    return m_baseVersion;
}

const std::string& ProductMetadata::getVersion() const
{
    return m_version;
}

void ProductMetadata::setDefaultHomePath(const std::string& defaultHomeFolder)
{
    m_defaultHomeFolder = defaultHomeFolder;
}

const std::string& ProductMetadata::getDefaultHomePath() const
{
    return m_defaultHomeFolder;
}

const std::vector<Tag> ProductMetadata::tags() const
{
    return m_tags;
}

void ProductMetadata::setBaseVersion(const std::string& baseVersion)
{
    m_baseVersion = baseVersion;
}

void ProductMetadata::setFeatures(const std::vector<std::string>& features)
{
    m_features = features;
}

const std::vector<std::string>& ProductMetadata::getFeatures() const
{
    return  m_features;
}
