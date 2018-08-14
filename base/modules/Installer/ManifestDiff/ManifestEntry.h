/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once


#include <string>
#include <vector>
#include <set>

namespace Installer
{
    namespace ManifestDiff
    {
        using Path = std::string;
        class ManifestEntry
        {
        public:
            explicit ManifestEntry(Path path);
            ManifestEntry& withSHA1(const std::string& hash);
            ManifestEntry& withSHA256(const std::string& hash);
            ManifestEntry& withSHA384(const std::string& hash);
            ManifestEntry& withSize(unsigned long size);

            unsigned long size() const;
            std::string sha1() const;
            std::string sha256() const;
            std::string sha384() const;
            Path path() const;

            bool operator<(const ManifestEntry& other) const;

            /**
             * Convert a Manifest path to a posix path (And remove leading ./)
             *
             * .\install.sh to install.sh
             * @param p
             * @return
             */
            static Path toPosixPath(const Path& p);
        private:
            unsigned long m_size;
            Path m_path;
            std::string m_sha1;
            std::string m_sha256;
            std::string m_sha384;
        };

        using ManifestEntryVector = std::vector<ManifestEntry>;
        using ManifestEntrySet = std::set<ManifestEntry>;
        using PathSet = std::set<Path>;
    }
}


