/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>
#include <vector>

class InclusionController
{
public:
    virtual ~InclusionController();

    /**
     *
     * @param scan
     * @param mountInfo
     * @param log    Log inclusions and exclusions here.
     */
    InclusionController();
    /**
     * Get the next path to start scanning from.
     */
    std::string getNextInclusion();
    /**
     * Should we scan this path?
     *
     * @param path
     */
    bool includePath(const std::string& path);
    /**
     * @return true if the path is one of the explicit inclusions.
     *
     * @param path
     */
    bool isExplicitInclusion(const std::string& path);

private:
    /**
     * Setup the explicit exclusions
     *
     * @param scan
     * @param log
     */
    void setupExplicitExclusions();
    /**
     * Determine if a path is explicitly excluded.
     *
     * @param path
     */
    bool isPathExplicitlyExcluded(const std::string& path);
    std::string m_currentInclusionPoint;
    /**
     * List of exclusions regex to always apply.
     */
    std::vector<std::string> m_explicitExclusions;


    /**
     * List of mount points that are excluded (wrong type).
     */
    std::vector<std::string> m_mountExclusions;
    /**
     * The current selected inclusion path is explicit (Only longer explicit
     * exclusions should apply). Otherwise it is a drive inclusion path, for which all
     * exclusions and all longer mount point exclusions should apply.
     */
    bool m_currentInclusionPathIsExplicit;
    /**
     * Full list of inclusion paths.
     */
    std::vector<std::string> m_inclusionPaths;
    /**
     * List of remaining starting points that were explicitly requested.
     */
    std::vector<std::string> m_pendingExplicitInclusionPaths;
    /**
     * List of remaining starting points for mounted drives.
     */
    std::vector<std::string> m_pendingMountInclusionPaths;
    /**
     * List of exclusions explicitly specified by scan.
     * Stems of full paths to exclude.
     */
    std::vector<std::string> m_explicitStemExclusions;
    /**
     * List of exclusions explicitly specified by scan.
     * Full paths to exclude.
     */
    std::vector<std::string> m_explicitFullPathExclusions;
    /**
     * Complete list of explicit inclusion paths.
     */
    std::vector<std::string> m_explicitInclusionPaths;
};
