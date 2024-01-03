// Copyright 2023 Sophos Limited. All rights reserved.
#pragma once

#include "Common/FileSystem/IFileSystem.h"

#include <map>
#include <string>

namespace Common::ConfigFile
{
    class ConfigFile
    {
    public:
        using line_t = std::string;
        using lines_t = std::vector<line_t>;
        using key_t = std::string;
        using value_t = std::string;

        ConfigFile(Common::FileSystem::IFileSystem* fs, const std::string& path, bool ignoreMissingFile=true);
        ConfigFile(const lines_t& lines);
        value_t at(const key_t& key) const;
        value_t get(const key_t& key, const value_t& defaultValue) const;
        bool getBoolean(const key_t& key, bool defaultValue) const;
        bool contains(const key_t& key) const;
    private:
        std::map<key_t, value_t> values_;
        void parseLine(const line_t& line);
        void parseLines(const lines_t& lines);

    };
}
