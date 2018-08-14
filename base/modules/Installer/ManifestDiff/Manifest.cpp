/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "Manifest.h"

#include <Common/UtilityImpl/iostr_utils.h>
#include <Common/UtilityImpl/StringUtils.h>

#include <fstream>
#include <limits>
#include <cassert>

using namespace Installer::ManifestDiff;

static void parseSHA256comment(const std::string& comment, ManifestEntry& info)
{
    assert(comment[0] == '#');
    if (comment.substr(1,7) != "sha256 ")
    {
        return;
    }
    std::string sha256 = comment.substr(8,72);
    info.withSHA256(sha256);
}

Manifest::Manifest(std::istream& file)
{
    file >> std::noskipws;

    std::string body;

    // look for the signature header
    file >> get_upto(body, "\n-----BEGIN SIGNATURE-----\n");

    std::istringstream in(body);

    in >> std::noskipws;

    while (in.good())
    {

        std::string path;
        unsigned long size;
        std::string checksum;
        char comment[100];
        switch (in.peek())
        {    // With an MS iostream, calling peek() on a stream which is in an eof state
            // sets the fail bit! So be careful to only call peek() once. GNU does it ok.
            case '#':
                // Need to work out if we have a sha-256 comment
                in.get(comment, 100, '\n');
                if (in.gcount() == 72 and !m_entries.empty())
                {
                    // Possibly SHA-256 comment
                    parseSHA256comment(comment, m_entries.back());
                }
                else if (in.fail())
                {
                    // comment longer than 100 bytes - never a SHA-256 comment.
                    in.clear(std::ios::failbit);
                }
                // Either a single \n or comment longer than 100 bytes.
                in.ignore(std::numeric_limits<int>::max(), '\n');
                continue;

            case '"':
                in >> std::expect('"') >> match(std::char_class_file, path) >> std::expect('"') >> std::expect(' ')
                   >> size >> std::expect(' ')
                   >> match(std::char_class_hex, checksum) >> std::expect('\n');
                m_entries.emplace_back(path);
                m_entries.back().withSHA1(checksum).withSize(size);
                continue;
            default:
                break;
        }
        break; // can't break in the default branch, so break here and other branches continue
    }
    in >> std::expect_eof;
}

Manifest Manifest::ManifestFromPath(const std::string& filepath)
{
    std::ifstream stream(filepath);
    return Manifest(stream);
}

unsigned long Manifest::size()
{
    return m_entries.size();
}
