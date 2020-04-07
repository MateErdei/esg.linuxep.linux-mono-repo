/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Exclusion.h"

#include "PathUtils.h"

#include <regex>

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
        if (path.at(0) == '/')
        {
            if (path.size() > 1 && path.compare(path.size()-2, 2, "/*") == 0)
            {
                std::string truncatedPath = path.substr(0, path.size() - 1);
                if (truncatedPath.find('?') == std::string::npos && truncatedPath.find('*') == std::string::npos)
                {
                    m_type = STEM;
                    m_exclusionPath = truncatedPath;
                    return;
                }
            }

            m_type = GLOB;
            m_exclusionPath = convertGlobToRegex(path);
        }
        else
        {
            if (path.size() > 1 && path.compare(0, 2, "*/") == 0)
            {
                std::string truncatedPath = path.substr(2, path.size() - 2);
                if (truncatedPath.find('?') == std::string::npos && truncatedPath.find('*') == std::string::npos)
                {
                    if (truncatedPath.find('/') == std::string::npos)
                    {
                        m_type = FILENAME;
                    }
                    else if (truncatedPath.at(truncatedPath.size()-1) == '/')
                    {
                        m_type = RELATIVE_STEM;
                    }
                    else
                    {
                        m_type = RELATIVE_PATH;
                    }
                    m_exclusionPath = "/" + truncatedPath;
                    return;
                }
            }

            m_type = RELATIVE_GLOB;
            m_exclusionPath = convertGlobToRegex("*/" + path);
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
        m_exclusionPath = "/" + path;

        if (path.find('/') == std::string::npos)
        {
            m_type = FILENAME;
        }
        else if (path.at(path.size()-1) == '/')
        {
            m_type = RELATIVE_STEM;
        }
        else
        {
            m_type = RELATIVE_PATH;
        }
    }
}

bool Exclusion::appliesToPath(const std::string& path) const
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
            std::regex pathRegex(m_exclusionPath);
            if (std::regex_match(path, pathRegex))
            {
                return true;
            }
            break;
        }
        case FILENAME:
        case RELATIVE_PATH:
        {
            if (PathUtils::endswith(path, m_exclusionPath))
            {
                return true;
            }
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

std::string Exclusion::convertGlobToRegex(const std::string& glob)
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

    return regexStr;
}