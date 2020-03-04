/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Exclusion.h"

#include "PathUtils.h"

using namespace avscanner::avscannerimpl;


Exclusion::Exclusion(const std::string& path)
    : m_exclusionPath(path)
{
    if (path.empty())
    {
        m_type = INVALID;
    }
    else if (path.find('?') != std::string::npos || path.find('*') != std::string::npos)
    {
        if (path.size() > 1 && path.compare(path.size()-2, 2, "/*") == 0)
        {
            std::string truncatedPath = path.substr(0, path.size() - 1);
            if (truncatedPath.find('?') != std::string::npos || truncatedPath.find('*') != std::string::npos)
            {
                m_type = GLOB;
            }
            else
            {
                m_type = STEM;
                m_exclusionPath = truncatedPath;
            }
        }
        else
        {
            m_type = GLOB;
        }
    }
    else if (path.at(0) == '/')
    {
        if (path.at(path.size()-1) == '/')
        {
            m_type = STEM;
        }
        else
        {
            m_type = FULLPATH;
        }
    }
    else
    {
        if (path.find('/') == std::string::npos)
        {
            m_type = FILENAME;
            m_exclusionPath = "/" + path;
        }
        else
        {
            m_type = INVALID;
        }
    }
}

bool Exclusion::appliesToPath(const std::string& path) const
{
    switch(m_type)
    {
        case STEM:
            if (PathUtils::startswith(path, m_exclusionPath))
            {
                return true;
            }
            break;
        case FULLPATH:
            if (path == m_exclusionPath)
            {
                return true;
            }
            break;
        default:
            break;
    }
    return false;
}

std::string Exclusion::path() const
{
    return m_exclusionPath;
}

ExclusionType Exclusion::type() const
{
    return m_type;
}
