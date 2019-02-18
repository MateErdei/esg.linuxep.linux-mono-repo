/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "Manifest.h"

#include <Common/UtilityImpl/StringUtils.h>
#include <Common/UtilityImpl/iostr_utils.h>

#include <cassert>
#include <fstream>
#include <limits>
#include <map>

using namespace Installer::ManifestDiff;

static void parseSHA256comment(const std::string& comment, ManifestEntry& info)
{
    assert(comment[0] == '#');
    if (comment.substr(1, 7) != "sha256 ")
    {
        return;
    }
    std::string sha256 = comment.substr(8, 72);
    info.withSHA256(sha256);
}

Manifest::Manifest(std::istream& file)
{
    if (!file.good())
    {
        // Count missing files as empty
        return;
    }

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
        { // With an MS iostream, calling peek() on a stream which is in an eof state
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
                in >> std::expect('"') >> match(std::char_class_file, path) >> std::expect('"') >> std::expect(' ') >>
                    size >> std::expect(' ') >> match(std::char_class_hex, checksum) >> std::expect('\n');
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
    if (filepath.empty())
    {
        return Manifest();
    }
    std::ifstream stream(filepath);
    return Manifest(stream);
}

unsigned long Manifest::size() const
{
    return m_entries.size();
}

ManifestEntrySet Manifest::entries() const
{
    return Installer::ManifestDiff::ManifestEntrySet(m_entries.begin(), m_entries.end());
}

PathSet Manifest::paths() const
{
    PathSet paths;
    for (const auto& entry : m_entries)
    {
        paths.insert(entry.path());
    }
    return paths;
}

ManifestEntrySet Manifest::calculateAdded(const Manifest& oldManifest) const
{
    PathSet oldPaths = oldManifest.paths();
    ManifestEntrySet added;
    for (const auto& entry : m_entries)
    {
        if (oldPaths.find(entry.path()) == oldPaths.end())
        {
            added.insert(entry);
        }
    }
    return added;
}

ManifestEntrySet Manifest::calculateRemoved(const Manifest& oldManifest) const
{
    // This is just the natural opposite of calculateAdded
    return oldManifest.calculateAdded(*this);
}

ManifestEntrySet Manifest::calculateChanged(const Manifest& oldManifest) const
{
    ManifestEntrySet changed;
    ManifestEntryMap oldMap = oldManifest.getEntriesByPath();

    for (const auto& entry : m_entries)
    {
        if (oldMap.count(entry.path()) > 0)
        {
            if (oldMap.at(entry.path()) != entry)
            {
                changed.insert(entry);
            }
        }
    }
    return changed;
}

Manifest::ManifestEntryMap Manifest::getEntriesByPath() const
{
    Manifest::ManifestEntryMap map;
    for (const auto& entry : m_entries)
    {
        map[entry.path()] = entry;
    }
    return map;
}
