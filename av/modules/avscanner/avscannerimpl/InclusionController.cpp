/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "InclusionController.h"

#include "datatypes/Print.h"

#include <algorithm>

bool startswith(const std::string& basestr, const std::string& teststr)
{
    if (teststr.size() > basestr.size())
    {
        // basestr can't start with a longer string
        return false;
    }
    return basestr.compare(0,teststr.size(),teststr) == 0;
}

InclusionController::~InclusionController()
{
}

/**
 *
 * @param scan
 * @param mountInfo
 * @param log    Log inclusions and exclusions here.
 */
InclusionController::InclusionController()
        :
        m_currentInclusionPathIsExplicit(false)
{
    /**
        I've chosen to do a OR operation between the device inclusions and the
        explicit inclusions:

        (/home + Optical + Removable) scans /home, /media/cdrom, /media/floppy etc

        The alternative is an AND operation:

        (/home + Optical + Removable) scans nothing.
        (/home + Network) scans smb, etc mounts under /home.

        Either may make sense in certain circumstances.
     */


    m_explicitExclusions = 0;

    // setup explicit include
    StringSet startingPoints = scan.getInclusions();
    for (StringSet::const_iterator it = startingPoints.begin();
         it != startingPoints.end();
         ++it)
    {
        m_pendingExplicitInclusionPaths.insert(*it);
        m_explicitInclusionPaths.insert(*it);
        log.directoryIncluded(*it);
    }

    // setup explicit exclude
    setupExplicitExclusions(scan,log);

    // Need to perform mount analysis
    bool includeHardDisc = scan.findScanObject(Common::NamedScan::Hard);
    bool includeOptical = scan.findScanObject(Common::NamedScan::Optical);
    bool includeRemovable = scan.findScanObject(Common::NamedScan::Removable);
    bool includeNetwork = scan.findScanObject(Common::NamedScan::Network);

    // For each mount point
    std::vector<Common::DeviceInfo::IMountPoint*> mountpoints = mountInfo.mountPoints();
    for (std::vector<Common::DeviceInfo::IMountPoint*>::const_iterator mp = mountpoints.begin();
         mp != mountpoints.end(); ++mp)
    {
        const std::string& mountpoint = (*mp)->mountPoint();

        // Specials must be excluded, even if we aren't worried about devices.
        // They must also be excluded, even if they are covered by another explicit exclusion
        // (to cover multi-level include/excludes)
        // e.g. / explicitly excluded
        //      /var explicitly included
        //      /var/proc special file system
        //     /var/proc still needs to be excluded
        if ((*mp)->isSpecial())
        {
            // excluding all system/pseudo filesystems
            // which need to be excluded from all scans including explicit scans
            m_explicitStemExclusions.insert(mountpoint);
            PRINT("Special exclusion:"<<mountpoint);
            log.mountExcluded(mountpoint);
        }
            // DEF64331 - Scheduled scans when /home is excluded, still /home/testuser/.gvfs is attempted to be read and fails with error
            // DEF82083 - SAV scheduled scan excludes root directory, but it stil prints out some mount points included
        else if ( isPathExplicitlyExcluded(mountpoint) )
        {
            PRINT("Mount not considered: ExplicitlyExcluded:"<<mountpoint);
            // Should we log the exclusion?
        }
        else
        {
            if ( (includeHardDisc && (*mp)->isHardDisc()) ||
                 (includeNetwork && (*mp)->isNetwork()) ||
                 (includeOptical && (*mp)->isOptical()) ||
                 (includeRemovable && (*mp)->isRemovable()) )
            {
                m_pendingMountInclusionPaths.insert(mountpoint);
                PRINT("MountInclusion:"<<mountpoint);
                log.mountIncluded(mountpoint);
            }
            else if (includeHardDisc || includeNetwork || includeOptical || includeRemovable)
            {
                m_mountExclusions.insert(mountpoint);
                PRINT("Exclusion:"<<mountpoint);
                log.mountExcluded(mountpoint);
            }
        }
    }

    // if no include paths at all specified then scan /
    if (m_pendingMountInclusionPaths.size() == 0 && m_pendingExplicitInclusionPaths.size() == 0)
    {
        m_pendingExplicitInclusionPaths.insert("/");
        PRINT("Inclusion:/");
        log.directoryIncluded("/");
    }

}

/**
 * Get the next path to start scanning from.
 */
std::string InclusionController::getNextInclusion()
{
    /*
     We do the mount inclusions first as they are more likely to be excluded.

     Consider:
     / - explicit exclude
     /tmp - explicit include
     /tmp - local fixed mount point
     ScanHarddrives true

     /tmp - mount point excluded by explicit exclude, also skipped as going to be scanned later
     /tmp - explicit include scanned

     DEF19710 happened when we had the operations the other way around:
     /tmp - explicit include skipped because /tmp going to be scanned latter
     /tmp - mount point excluded by explicit exclude
     */

    if (m_pendingMountInclusionPaths.size() > 0)
    {
        m_currentInclusionPathIsExplicit = false;
        m_currentInclusionPoint = m_pendingMountInclusionPaths.pop_front();

        // DEF64331 - Scheduled scans when /home is excluded, still /home/testuser/.gvfs is attempted to be read and fails with error
        if ( isPathExplicitlyExcluded(m_currentInclusionPoint) )
        {
            PRINT(m_currentInclusionPoint << " excluded");
            m_currentInclusionPoint = getNextInclusion();
        }
        else
        {
            PRINT(m_currentInclusionPoint << " not excluded");
        }
        PRINT("next inclusion mount    = "<<m_currentInclusionPoint);
    }
    else if (m_pendingExplicitInclusionPaths.size() > 0)
    {
        m_currentInclusionPathIsExplicit = true;
        m_currentInclusionPoint = m_pendingExplicitInclusionPaths.pop_front();
        PRINT("next inclusion explicit = "<<m_currentInclusionPoint);
    }
    else
    {
        // ??? Should we use an exception here instead?
        m_currentInclusionPoint = "FINISHED";
        PRINT("Inclusions finished = "<<m_currentInclusionPoint);
    }
    return m_currentInclusionPoint;
}

/**
 * Should we scan this path?
 *
 * @param path
 */
bool InclusionController::includePath(const std::string& path)
{
    // First - if any of the regex match then we exclude the file
    if (m_explicitExclusions->match(path))
    {
        PRINT("Explicit exclusion:"<<path);
        return false;
    }

    std::set<std::string>::const_iterator exclude;
    for (exclude = m_explicitFullPathExclusions.begin();exclude != m_explicitFullPathExclusions.end(); ++exclude)
    {
        if (path == *exclude)
        {
            return false;
        }
    }

    // If we are on an explicit include then only exclude if a longer explicit include is provided.
    // then check stems
    for (exclude = m_explicitStemExclusions.begin();exclude != m_explicitStemExclusions.end(); ++exclude)
    {
        if (startswith(path,*exclude))
        {
            if (m_currentInclusionPathIsExplicit)
            {
                // if the include was explicit then we need to check if the include is longer than the exclude.
                if (m_currentInclusionPoint.size() < exclude->size())
                {
                    // The exclude is the longer/same length as include therefore applies against this file
                    PRINT("ExplicitStemExclusion longer than include:"<<path);
                    return false;
                }
            }
            else
            {
                // Always apply excludes against mount point inclusions
                PRINT("ExplicitStemExclusion against mount point inclusion:"<<path);
                return false;
            }
        }
    }

    // Check we aren't about to recurse into somewhere we are going to go to:
    if (std::find(m_pendingExplicitInclusionPaths.begin(), m_pendingExplicitInclusionPaths.end(), path))
    {
        PRINT("PendingExplicitInclusion:"<<path);
        return false;
    }
    if (m_pendingMountInclusionPaths.contains(path))
    {
        PRINT("PendingMountInclusion:"<<path);
        return false;
    }

    // if we are on an explicit include then we are done
    if (m_currentInclusionPathIsExplicit)
    {
        return true;
    }

    // Otherwise we need to consider excluded mount points
    for (exclude = m_mountExclusions.begin();exclude != m_mountExclusions.end(); ++exclude)
    {
        if (startswith(path,*exclude))
        {
            // Consider scanning network only:
            // We might have mount excluded e.g. / and included /mnt/nfs
            // But we do want to scan the volume /mnt/nfs

            // we need to check if the include is longer than the exclude:
            if (m_currentInclusionPoint.size() < exclude->size())
            {

                PRINT("MountPointExclusion:"<<path);
                return false;
            }
        }
    }

    return true;
}



/**
 * Setup the explicit exclusions
 *
 * @param scan
 * @param log
 */
void InclusionController::setupExplicitExclusions(const Common::NamedScan::INamedScan& scan, NamedScanLog& log)
{
    //  setup exclude paths
    StringSet excludes = scan.getExclusions();
    StringSet::const_iterator exclude;

    m_explicitExclusions = Common::Regex::IRegexFactory::factory().newRegexList();

    for (exclude = excludes.begin(); exclude != excludes.end(); ++exclude)
    {
        if (exclude->size() == 0)
        {
            // ignore empty exclusions
        }
        else if (exclude->find_first_of("*?") != std::string::npos)
        {
            // If the paths are have glob characters (? and *) then treat as entire globs

            try
            {
                m_explicitExclusions->addGlob(*exclude);
                PRINT("Exclusion:"<<*exclude);
                log.directoryExcluded(*exclude);
            }
            catch (const Common::Regex::IRegexException& e)
            {
                PRINT("Failed to add named scan exclusion: "<<*exclude);
            }
        }
        else if ((*exclude)[0] == '/')
        {
            // If they start with / then they are full path exclusions.
            // /foo
            //   -> /foo/    Stem - excludes directory
            //   -> /foo     Full path - excludes file

            // /foo/ -> /foo/   Stem - excludes directory

            PRINT("Exclusion Fullpath:"<<*exclude);
            log.directoryExcluded(*exclude);
            if ((*exclude)[exclude->size()-1] != '/')
            {
                m_explicitFullPathExclusions.insert(*exclude);

                m_explicitStemExclusions.insert(*exclude + "/");
                log.directoryExcluded(*exclude + "/");
            }
            else
            {
                m_explicitStemExclusions.insert(*exclude);
            }
        }
        else
        {
            // If they don't start with / then they refer to tails
            // Assume if they exclude "eicar.com", then don't also want to exclude "NOTeicar.com"
            std::string glob = "*/";
            glob.append(*exclude);
            try
            {
                m_explicitExclusions->addGlob(glob);
                PRINT("Exclusion:"<<glob);
                log.directoryExcluded(glob);
            }
            catch (const Common::Regex::IRegexException& e)
            {
                PRINT("Failed to add named scan exclusion: "<<*exclude);
            }
        }
    }

    // And also add regular expressions for excluded extensions
    StringSet excludedExtensions = scan.getExtensionExclusions();
    StringSet::const_iterator excludedExtension;
    for (excludedExtension = excludedExtensions.begin(); excludedExtension != excludedExtensions.end(); ++excludedExtension)
    {
        // Regular expressions are searched for, so therefore we don't need .* at the start
        std::string expression = "\\." + *excludedExtension + "$";
        try
        {
            m_explicitExclusions->addRegularExpression(expression,true,false);
            PRINT("Exclusion:"<<expression);
            log.directoryExcluded(expression);
        }
        catch (const Common::Regex::IRegexException& e)
        {
            PRINT("Failed to add named scan exclusion: "<<expression);
        }
    }
}


/**
 * @return true if the path is one of the explicit inclusions.
 *
 * @param path
 */
bool InclusionController::isExplicitInclusion(const std::string& path)
{
    return m_explicitInclusionPaths.contains(path);
}


/**
 * Determine if a path is explicitly excluded.
 *
 * @param path
 */
bool InclusionController::isPathExplicitlyExcluded(const std::string& path)
{
    std::set<std::string>::const_iterator exclude;
    for (exclude = m_explicitStemExclusions.begin();exclude != m_explicitStemExclusions.end(); ++exclude)
    {
        if (startswith(path,*exclude))
        {
            return true;
        }
    }

    for (exclude = m_explicitFullPathExclusions.begin();exclude != m_explicitFullPathExclusions.end(); ++exclude)
    {
        if (path == *exclude)
        {
            return true;
        }
    }

    if (m_explicitExclusions->match(path))
    {
        return true;
    }

    return false;
}