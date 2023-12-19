// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "Exclusion.h"
#include "Logger.h"

#include "common/PathUtils.h"

#include <regex>

using namespace common;

namespace
{
    void escapeRegexMetaCharacters(std::string& text)
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

    bool applyRegexToPath(const char* path, const Exclusion::regex_t& regex)
    {
#ifdef EXCLUSION_USE_RE2
        return RE2::FullMatch(path, *regex);
#else
        return std::regex_match(path, regex);
#endif
    }

    Exclusion::regex_t compileRegex(const std::string regexStr)
    {
#ifdef EXCLUSION_USE_RE2
        return std::make_shared<RE2>(regexStr);
#else /* ! EXCLUSION_USE_RE2 */
        return std::regex(regexStr);
#endif /* EXCLUSION_USE_RE2 */
    }
}

std::string ExclusionBase::path() const
{
    return m_exclusionPath.string_;
}

std::string ExclusionBase::displayPath() const
{
    return m_exclusionDisplayPath;
}

ExclusionType ExclusionBase::type() const
{
    return m_type;
}

ExclusionBase::ExclusionBase(const std::string& path):
        m_exclusionPath(path),
        m_exclusionDisplayPath(path)
{
}

Exclusion::Exclusion(const std::string& path)
    : ExclusionBase(path)
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

    // if exclusionPath is '*/' skip this
    // if exclusionPath starts with '*/' remove the '*/'
    if (exclusionPath.size() > 2 && exclusionPath.compare(0, 2, "*/") == 0)
    {
        exclusionPath = exclusionPath.substr(2, exclusionPath.size() - 2);
    }

    if (exclusionPath.find('?') != std::string::npos || exclusionPath.find('*') != std::string::npos)
    {
        // Put back the * after /* that we just removed - needed for glob matches
        if (exclusionPath.at(exclusionPath.size()-1) == '/')
        {
            exclusionPath = exclusionPath + "*";
        }

        bool startsWithStar = exclusionPath.at(0) == '*';

        if (startsWithStar && exclusionPath.find('?', 1) == std::string::npos && exclusionPath.find('*', 1) == std::string::npos)
        {
            // No other glob chars after first char
            // so can be done as suffix instead
            m_type = SUFFIX;
            m_exclusionPath = CachedPath{exclusionPath.substr(1)};
        }
        else if (exclusionPath.at(0) == '/' || startsWithStar)
        {
            // Globs that start with * or / are absolute globs
            m_type = GLOB;
            m_exclusionPath = CachedPath{exclusionPath};
            m_pathRegex = convertGlobToRegex(m_exclusionPath);
        }
        else
        {
            // Relative globs need to have */ added to the front to match inside paths
            m_type = RELATIVE_GLOB;
            m_exclusionPath = CachedPath{"*/" + exclusionPath};
            m_pathRegex = convertGlobToRegex(m_exclusionPath);
        }
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


Exclusion::Exclusion(const std::vector<Exclusion>& exclusions)
    : ExclusionBase("<multi-glob>")
{
    m_type = MULTI_GLOB;
    for (const auto& excl : exclusions)
    {
        if (excl.isGlobExclusion())
        {
            if (!pathRegexString_.empty())
            {
                pathRegexString_.push_back('|');
            }
            pathRegexString_ += excl.pathRegexString_;
        }
    }
    LOGDEBUG("Creating multi-regex exclusion from " << pathRegexString_);
    m_exclusionPath = CachedPath{pathRegexString_};
    m_exclusionDisplayPath = pathRegexString_;
    m_pathRegex = compileRegex(pathRegexString_);
}

auto Exclusion::appliesToPath(const CachedPath& path, bool isDirectory, bool isFile) const -> bool
{
    switch(m_type)
    {
        case STEM:
        {
            if (common::PathUtils::startswith(path, m_exclusionPath))
            {
                return true;
            }
            //We also want stem exclusions to apply to directories
            //If we are passed a directory path we exclude it if it resolves to the same place as the exclusion.
            if (isDirectory && path.path().lexically_relative(m_exclusionPath.path()) == ".")
            {
                return true;
            }
            break;
        }
        case RELATIVE_STEM:
        {
            return common::PathUtils::contains(path, m_exclusionPath);
        }
        case SUFFIX:
        {
            return common::PathUtils::endswith(path, m_exclusionPath);
        }
        case FILENAME:
        {
            // FILENAME only applies to files, not directories or unknown
            if (!isFile)
            {
                break;
            }
            [[fallthrough]];
        }
        case RELATIVE_PATH:
        {
            return common::PathUtils::endswith(path, m_exclusionPath);
        }
        case FULLPATH:
        {
            // Full paths shouldn't match directories
            if (path == m_exclusionPath && isFile)
            {
                return true;
            }
            break;
        }
        case GLOB:
        case RELATIVE_GLOB:
        case MULTI_GLOB:
        {
            if (applyRegexToPath(path.c_str(), m_pathRegex))
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

auto Exclusion::appliesToPath(const fs::path& path, bool isDirectory, bool isFile) const -> bool
{
    CachedPath p{path};
    return appliesToPath(p, isDirectory, isFile);
}

// isDirectory defaults to false
bool Exclusion::appliesToPath(const fs::path& path, bool isDirectory) const
{
    if (isDirectory)
    {
        return appliesToPath(path, true, false);
    }
    return appliesToPath(path, false, true);
}

std::string Exclusion::convertGlobToRegexString(const CachedPath& glob)
{
    std::string regexStr = glob.string_;
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

Exclusion::regex_t Exclusion::convertGlobToRegex(const CachedPath& glob)
{
    pathRegexString_ = convertGlobToRegexString(glob);
    return compileRegex(pathRegexString_);
}

bool Exclusion::operator==(const Exclusion& rhs) const
{
    return m_exclusionPath == rhs.m_exclusionPath &&
           m_exclusionDisplayPath == rhs.m_exclusionDisplayPath &&
           m_type == rhs.m_type;
}

bool Exclusion::operator<(const Exclusion& rhs) const
{
    if (m_type != rhs.m_type)
    {
        return m_type < rhs.m_type;
    }
    const auto size = m_exclusionDisplayPath.size();
    const auto rhsSize = rhs.m_exclusionDisplayPath.size();
    if (size != rhsSize)
    {
        // Shorter exclusions first
        return size < rhsSize;
    }

    return m_exclusionDisplayPath < rhs.m_exclusionDisplayPath;
}

bool Exclusion::isGlobExclusion() const
{
    return m_type == GLOB || m_type == RELATIVE_GLOB || m_type == MULTI_GLOB;
}
