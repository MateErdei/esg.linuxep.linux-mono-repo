/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once


#include <string>
#include <vector>

namespace Installer
{
    namespace ManifestDiff
    {
        class ManifestEntry
        {
        public:
            explicit ManifestEntry(std::string path);
            ManifestEntry& withSHA1(const std::string& hash);
            ManifestEntry& withSHA256(const std::string& hash);
            ManifestEntry& withSHA384(const std::string& hash);
            ManifestEntry& withSize(unsigned long size);

            unsigned long size();
            std::string sha1();
            std::string sha256();
            std::string sha384();
            std::string path();

            /**
             * Convert a Manifest path to a posix path (And remove leading ./)
             *
             * .\install.sh to install.sh
             * @param p
             * @return
             */
            static std::string toPosixPath(const std::string& p);
        private:
            unsigned long m_size;
            std::string m_path;
            std::string m_sha1;
            std::string m_sha256;
            std::string m_sha384;
        };

        using ManifestEntryVector = std::vector<ManifestEntry>;
    }
}


