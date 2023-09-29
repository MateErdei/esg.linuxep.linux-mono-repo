// Copyright 2023 Sophos Limited. All rights reserved.

// Copied from esg.winep.sau/src/Verify/file_info.h

#pragma once

#include <iostream>
#include <string>
#include <list>

namespace manifest
{

    // This class represents the information about a file
    class file_info
    {
    public:
        using path_t = std::string;
        [[nodiscard]] path_t path() const { return path_; }
        [[nodiscard]] unsigned long size() const { return size_; }
        [[nodiscard]] std::string sha1() const { return sha1_; }
        [[nodiscard]] std::string sha256() const { return sha256_; }
        [[nodiscard]] std::string sha384() const { return sha384_; }

        // the current file syntax means we *always* have sha1
        [[nodiscard]] bool has_sha1() const { return !sha1_.empty(); }
        [[nodiscard]] bool has_sha256() const { return !sha256_.empty(); }
        [[nodiscard]] bool has_sha384() const { return !sha384_.empty(); }

        file_info& sha256(const std::string &sha256);
        file_info& sha384(const std::string &sha384);

        typedef enum { file_ok, file_invalid, file_missing } verify_result;

        [[nodiscard]] verify_result verify_file(const path_t& root_path) const;

        file_info();
        file_info(path_t path, unsigned long size, std::string sha1);

    private:
        path_t path_;           // relative path and name
        unsigned long size_;    // size of the file in bytes
        std::string sha1_;      // ascii hex representation of sha1 checksum of the file
        std::string sha256_;    // ascii hex representation of sha256 checksum of the file
        std::string sha384_;    // ascii hex representation of sha384 checksum of the file
        int algorithms_;
    };
}
