// Copyright 2023 Sophos Limited. All rights reserved.
#include "ConfigFile.h"

#include "Common/FileSystem/IFileNotFoundException.h"
#include "Common/UtilityImpl/StringUtils.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <iostream>
#include <locale>

#define PRINT(X) std::cerr << X << '\n'

using namespace Common::ConfigFile;

ConfigFile::ConfigFile(Common::FileSystem::IFileSystem* fs, const std::string& filePath)
{
    assert(fs != nullptr);
    try
    {
        auto contents = fs->readLines(filePath);
        parseLines(contents);
    }
    catch (const Common::FileSystem::IFileNotFoundException&)
    {
        // Ignore
    }
}

ConfigFile::ConfigFile(const std::vector<std::string>& lines)
{
    parseLines(lines);
}

void ConfigFile::parseLines(const std::vector<std::string>& lines)
{
    for (const auto& line : lines)
    {
        parseLine(line);
    }
}

void ConfigFile::parseLine(const std::string& line)
{
    auto pos = line.find('=');
    if (pos == std::string::npos)
    {
        return;
    }
    auto key = line.substr(0, pos);
    auto value = line.substr(pos+1);
    key = Common::UtilityImpl::StringUtils::trim(key);
    value = Common::UtilityImpl::StringUtils::lTrim(value);
    values_[key] = value;
}

std::string ConfigFile::at(const std::string& key) const
{
    return values_.at(key);
}

std::string ConfigFile::get(const std::string& key, const std::string& defaultValue) const
{
    try
    {
        return at(key);
    }
    catch (const std::out_of_range&)
    {
        return defaultValue;
    }
}

bool ConfigFile::getBoolean(const std::string& key, bool defaultValue) const
{
    try
    {
        auto value = at(key);
        return (value == "true");
    }
    catch (const std::out_of_range&)
    {
        return defaultValue;
    }
}

bool ConfigFile::contains(const std::string& key) const
{
    return values_.count(key) > 0;
}
