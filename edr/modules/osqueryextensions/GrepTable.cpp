// Copyright 2023 Sophos Limited. All rights reserved.

#include "GrepTable.h"

#include "ConstraintHelpers.h"
#include "Logger.h"

#include "Common/FileSystem/IFileSystem.h"

#include <fstream>
namespace OsquerySDK
{

    void GrepTable::GrepFolder(
        const std::string& filePath,
        const std::string& grepPath,
        const std::string& pattern,
        OsquerySDK::TableRows& results,
        OsquerySDK::QueryContextInterface& context)
    {
        auto fs = Common::FileSystem::fileSystem();
        std::vector<std::string> paths = fs->listFilesAndDirectories(filePath);
        for (auto const& subPath : paths)
        {
            GrepFile(grepPath, subPath, pattern, results, context);
        }
    }

    void GrepTable::GrepFile(
        const std::string& grepPath,
        const std::string& filePath,
        const std::string& pattern,
        OsquerySDK::TableRows& results,
        OsquerySDK::QueryContextInterface& context)
    {
        try
        {
            auto fs = Common::FileSystem::fileSystem();

            auto fileStream = fs->openFileForRead(filePath);
            for (std::string line; std::getline(*fileStream, line);)
            {
                if (line.size() < MAX_LINE_LENGTH && (pattern.empty() || line.find(pattern, 0) != std::string::npos))
                {
                    OsquerySDK::TableRow row;
                    if (context.IsColumnUsed("path"))
                    {
                        row["path"] = grepPath;
                    }
                    if (context.IsColumnUsed("pattern"))
                    {
                        row["pattern"] = pattern;
                    }
                    if (context.IsColumnUsed("filepath"))
                    {
                        row["filepath"] = filePath;
                    }
                    if (context.IsColumnUsed("line"))
                    {
                        row["line"] = line;
                    }
                    results.push_back(std::move(row));
                }
            }
        }
        catch (const std::exception& e)
        {
            std::string err = "Failed to grep file " + filePath + ". Error: " + e.what();
            LOGERROR(err);
        }
    }

    OsquerySDK::TableRows GrepTable::Generate(OsquerySDK::QueryContextInterface& context)
    {
        ConstraintHelpers constraintHelpers;
        LOGDEBUG(constraintHelpers.BuildQueryLogMessage(context, GetName()));
        OsquerySDK::TableRows results;

        std::set<std::string> pathConstraints = context.GetConstraints("path", OsquerySDK::ConstraintOperator::EQUALS);
        std::set<std::string> patternConstraints =
            context.GetConstraints("pattern", OsquerySDK::ConstraintOperator::EQUALS);

        if (pathConstraints.size() != 1)
        {
            std::string errorMessage = "Invalid grep query constraints. One path constraint should be provided.";
            throw std::runtime_error(errorMessage);
        }

        if (patternConstraints.size() > 1)
        {
            std::string errorMessage =
                "Invalid grep query constraints. Only one pattern constraint should be provided.";
            throw std::runtime_error(errorMessage);
        }

        std::string pattern { "" };
        const std::string path = *pathConstraints.begin();

        if (patternConstraints.size())
        {
            pattern = *patternConstraints.begin();
        }
        auto fs = Common::FileSystem::fileSystem();
        if (!fs->isFileOrDirectory(path))
        {
            LOGWARN("Failed to grep. Path does not exist");
            return results;
        }

        if (fs->isDirectory(path))
        {
            GrepFolder(path, path, pattern, results, context);
        }
        else
        {
            GrepFile(path, path, pattern, results, context);
        }

        return results;
    }
} // namespace OsquerySDK
