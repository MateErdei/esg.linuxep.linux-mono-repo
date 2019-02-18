/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <set>
#include <string>
#include <tuple>
#include <vector>

namespace Installer
{
    namespace ManifestDiff
    {
        using Path = std::string;
        class ManifestEntry
        {
        public:
            using entry_size_t = unsigned long;
            using hash_t = std::string;

            ManifestEntry();
            explicit ManifestEntry(Path path);
            ManifestEntry& withSHA1(const hash_t& hash);
            ManifestEntry& withSHA256(const hash_t& hash);
            ManifestEntry& withSHA384(const hash_t& hash);
            ManifestEntry& withSize(entry_size_t size);

            entry_size_t size() const;
            hash_t sha1() const;
            hash_t sha256() const;
            hash_t sha384() const;
            Path path() const;

            bool operator<(const ManifestEntry& other) const;
            bool operator==(const ManifestEntry& other) const;
            bool operator!=(const ManifestEntry& other) const;

            /**
             * Convert a Manifest path to a posix path (And remove leading ./)
             *
             * .\install.sh to install.sh
             * @param p
             * @return
             */
            static Path toPosixPath(const Path& p);

        private:
            entry_size_t m_size;
            Path m_path;
            hash_t m_sha1;
            hash_t m_sha256;
            hash_t m_sha384;

            inline std::tuple<Path, entry_size_t, hash_t, hash_t, hash_t> tie_members() const noexcept
            {
                return std::tie(m_path, m_size, m_sha1, m_sha256, m_sha384);
            }
        };

        using ManifestEntryVector = std::vector<ManifestEntry>;
        using ManifestEntrySet = std::set<ManifestEntry>;
        using PathSet = std::set<Path>;
    } // namespace ManifestDiff
} // namespace Installer
