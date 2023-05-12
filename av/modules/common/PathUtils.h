// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

#include <utility>

#include "datatypes/sophos_filesystem.h"

namespace fs = sophos_filesystem;

namespace common
{
    class CachedPath
    {
    private:
        mutable fs::path path_;
        mutable bool pathValid_;

    public:
        explicit CachedPath(std::string filePath)
            :
            pathValid_(false),
            string_(std::move(filePath))
        {
        }

        explicit CachedPath(const fs::path& filePath)
            :
            path_(filePath),
            pathValid_(true),
            string_(filePath.string())
        {
        }

        CachedPath(const CachedPath&) = default;
        CachedPath& operator=(const CachedPath&) = default;
        CachedPath& operator=(const std::string& s)
        {
            string_ = s;
            path_ = fs::path{s};
            return *this;
        }

        std::string string_;

        bool operator==(const CachedPath& rhs) const
        {
            return path_ == rhs.path_;
        }

        [[nodiscard]] const char* c_str() const noexcept
        {
            return string_.c_str();
        }

        [[nodiscard]] fs::path path() const
        {
            if (!pathValid_)
            {
                path_ = string_;
                pathValid_ = true;
            }
            return path_;
        }

    };

    class PathUtils
    {
    public:
        /**
         * Return true if a is longer than b.
         * @param a
         * @param b
         * @return
         */
        static bool longer(const fs::path& a, const fs::path& b)
        {
            return a.string().size() > b.string().size();
        }

        static bool startswith(const fs::path& p, const fs::path& value)
        {
            return p.string().rfind(value.string(), 0) == 0;
        }

        static bool startswith(const CachedPath& p, const CachedPath& value)
        {
            return p.string_.rfind(value.string_, 0) == 0;
        }

        static bool contains(const fs::path& p, const fs::path& value)
        {
            return p.string().find(value.string(), 0) != std::string::npos;
        }

        static bool contains(const CachedPath& p, const CachedPath& value)
        {
            return p.string_.find(value.string_, 0) != std::string::npos;
        }

        static bool endswith(const fs::path& p, const fs::path& value)
        {
            if (p.string().length() >= value.string().length()) {
                return (0 == p.string().compare(p.string().length() - value.string().length(), value.string().length(), value));
            } else {
                return false;
            }
        }

        static bool endswith(const CachedPath& p, const CachedPath& value)
        {
            const auto p_length = p.string_.length();
            const auto value_length = value.string_.length();
            if (p_length >= value_length) {
                return (0 == p.string_.compare(p_length - value_length, value_length, value.string_));
            } else {
                return false;
            }
        }

        static std::string appendForwardSlashToPath(const sophos_filesystem::path& p)
        {
            if (p.string().back() != '/')
            {
                return p.string() + "/";
            }
            return p.string();
        }

        static std::string removeForwardSlashFromPath(const sophos_filesystem::path& p)
        {
            if (p.string().back() == '/')
            {
                return p.string().substr(0, p.string().size()-1);
            }
            return p.string();
        }

        static std::string removeTrailingDotsFromPath(const sophos_filesystem::path& p)
        {
            std::string path = p.string();
            while(path.back() == '.')
            {
                 path.pop_back();
            }
            return path;
        }

        static bool isNonNormalisedPath(const sophos_filesystem::path& p)
        {
            if(contains(p, "//"))
            {
                return true;
            }

            auto pos = p.string().find_last_of('/');
            const auto leaf = p.string().substr(pos+1);

            if(leaf == "." || leaf == "..")
            {
                return true;
            }

            // the last part of each path object is always a "." so don't check the last part
            for (auto part = p.begin(); part != std::prev(p.end()); part++)
            {
                if (part->string() == ".." || part->string() == ".")
                {
                    return true;
                }
            }

            return false;
        }

        static std::string lexicallyNormal(const sophos_filesystem::path& p);

    };
}



