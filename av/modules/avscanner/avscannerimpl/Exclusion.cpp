/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Exclusion.h"

#include "PathUtils.h"

#include <regex>

using namespace avscanner::avscannerimpl;


Exclusion::Exclusion(const std::string& path)
{
    std::string exclusionPath(path);

    if (exclusionPath.empty())
    {
        m_type = INVALID;
        return;
    }
    
    // if exclusionPath ends with '/*' remove the '*'
    if (exclusionPath.size() > 1 && exclusionPath.compare(exclusionPath.size()-2, 2, "/*") == 0)
    {
        exclusionPath = exclusionPath.substr(0, exclusionPath.size() - 1);
    }

    // if exclusionPath starts with '*/' remove the '*/'
    if (exclusionPath.size() > 1 && exclusionPath.compare(0, 2, "*/") == 0)
    {
        exclusionPath = exclusionPath.substr(2, exclusionPath.size() - 2);
    }

    if (exclusionPath.find('?') != std::string::npos || exclusionPath.find('*') != std::string::npos)
    {
        if (exclusionPath.at(exclusionPath.size()-1) == '/')
        {
            exclusionPath = exclusionPath + "*";
        }

        if (exclusionPath.at(0) == '/')
        {
            m_type = GLOB;
            m_exclusionPath = exclusionPath;
        }
        else
        {
            m_type = RELATIVE_GLOB;
            m_exclusionPath = "*/" + exclusionPath;
        }
        m_pathRegex = convertGlobToRegex(m_exclusionPath);
    }
    else if (exclusionPath.at(0) == '/')
    {
        m_exclusionPath = exclusionPath;

        if (exclusionPath.at(exclusionPath.size()-1) == '/')
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
        m_exclusionPath = "/" + exclusionPath;

        if (exclusionPath.find('/') == std::string::npos)
        {
            m_type = FILENAME;
        }
        else if (exclusionPath.at(exclusionPath.size()-1) == '/')
        {
            m_type = RELATIVE_STEM;
        }
        else
        {
            m_type = RELATIVE_PATH;
        }
    }
}

// isDirectory defaults to false
bool Exclusion::appliesToPath(const std::string& path, bool isDirectory) const
{
    switch(m_type)
    {
        case STEM:
        {
            if (PathUtils::startswith(path, m_exclusionPath))
            {
                return true;
            }
            break;
        }
        case RELATIVE_STEM:
        {
            if (PathUtils::contains(path, m_exclusionPath))
            {
                return true;
            }
            break;
        }
        case FILENAME:
        {
            if (isDirectory)
            {
                break;
            }
            [[fallthrough]];
        }
        case RELATIVE_PATH:
        {
            if (PathUtils::endswith(path, m_exclusionPath))
            {
                return true;
            }
            break;
        }
        case FULLPATH:
        {
            if (path == m_exclusionPath)
            {
                return true;
            }
            break;
        }
        case GLOB:
        case RELATIVE_GLOB:
        {
            if (std::regex_match(path, m_pathRegex))
            {
                return true;
            }
            break;
        }

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

void Exclusion::escapeRegexMetaCharacters(std::string& text)
{
    std::string buffer;
    buffer.reserve(text.size());
    for(size_t pos = 0; pos != text.size(); ++pos) {
        switch(text[pos]) {
            case '\\': buffer.append("\\\\");          break;
            case '^': buffer.append("\\^");            break;
            case '$': buffer.append("\\$");            break;
            case '.': buffer.append("\\.");            break;
            case '|': buffer.append("\\|");            break;
            case '+': buffer.append("\\+");            break;
            case '(': buffer.append("\\(");            break;
            case ')': buffer.append("\\)");            break;
            case '{': buffer.append("\\{");            break;
            case '[': buffer.append("\\[");            break;
            default: buffer.push_back(text[pos]);         break;
        }
    }
    text.swap(buffer);
}

std::regex Exclusion::convertGlobToRegex(const std::string& glob)
{
    std::string regexStr = glob;
    escapeRegexMetaCharacters(regexStr);

    std::size_t found = regexStr.find_first_of('?');
    while (found != std::string::npos)
    {
        regexStr[found] = '.';
        found = regexStr.find_first_of('?', found+1);
    }

    found = regexStr.find_first_of('*');
    while (found != std::string::npos)
    {
        regexStr.insert(found, 1, '.');
        found = regexStr.find_first_of('*', found+2);
    }

    return std::regex(regexStr);
}