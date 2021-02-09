/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "datatypes/sophos_filesystem.h"

namespace fs = sophos_filesystem;

namespace common
{
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

        static bool contains(const fs::path& p, const fs::path& value)
        {
            return p.string().find(value.string(), 0) != std::string::npos;
        }

        static bool endswith(const fs::path& p, const fs::path& value)
        {
            if (p.string().length() >= value.string().length()) {
                return (0 == p.string().compare(p.string().length() - value.string().length(), value.string().length(), value));
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

        static bool isNonNormalisedPath(const sophos_filesystem::path& p);

        static std::string lexicallyNormal(const sophos_filesystem::path& p);

    };
}



