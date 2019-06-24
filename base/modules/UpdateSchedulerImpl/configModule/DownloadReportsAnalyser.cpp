/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "DownloadReportsAnalyser.h"

#include "../Logger.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/UtilityImpl/StringUtils.h>

#include <algorithm>
#include <cassert>

using namespace Common::UtilityImpl;
namespace
{
    using MessageInsert = UpdateSchedulerImpl::configModule::MessageInsert;
    using UpdateEvent = UpdateSchedulerImpl::configModule::UpdateEvent;
    using EventMessageNumber = UpdateSchedulerImpl::configModule::EventMessageNumber;
    using UpdateStatus = UpdateSchedulerImpl::configModule::UpdateStatus;

    void buildMessagesInsertFromDownloadReport(
        std::vector<MessageInsert>* messages,
        const SulDownloader::suldownloaderdata::DownloadReport& report)
    {
        for (auto& product : report.getProducts())
        {
            if (!product.errorDescription.empty())
            {
                messages->emplace_back(product.name, product.errorDescription);
            }
        }
    }

    void buildMessagesFromReport(
        std::vector<MessageInsert>* messages,
        const SulDownloader::suldownloaderdata::DownloadReport& report)
    {
        buildMessagesInsertFromDownloadReport(messages, report);
        // if no specific error associated with a product is present, report the general error.
        if (messages->empty())
        {
            messages->emplace_back("", report.getDescription());
        }
    }

    void handlePackageSourceMissing(UpdateEvent* event, const SulDownloader::suldownloaderdata::DownloadReport& report)
    {
        std::vector<std::string> splittedEntries =
            Common::UtilityImpl::StringUtils::splitString(report.getDescription(), ";");
        if (splittedEntries.empty())
        {
            event->MessageNumber = EventMessageNumber::SINGLEPACKAGEMISSING;
            return;
        }
        if (splittedEntries.size() == 1)
        {
            event->MessageNumber = EventMessageNumber::SINGLEPACKAGEMISSING;
            event->Messages.emplace_back(splittedEntries[0], "");
        }
        else
        {
            event->MessageNumber = EventMessageNumber::MULTIPLEPACKAGEMISSING;
            for (auto& entry : splittedEntries)
            {
                event->Messages.emplace_back(entry, "");
            }
        }
    }

    UpdateEvent extractEventFromSingleReport(const SulDownloader::suldownloaderdata::DownloadReport& report)
    {
        UpdateEvent event;
        switch (report.getStatus())
        {
            case SulDownloader::suldownloaderdata::WarehouseStatus::SUCCESS:
                event.MessageNumber = EventMessageNumber::SUCCESS;
                break;
            case SulDownloader::suldownloaderdata::WarehouseStatus::CONNECTIONERROR:
                event.MessageNumber = EventMessageNumber::CONNECTIONERROR;
                // no package name, just the error details
                event.Messages.emplace_back("", report.getDescription());
                break;
            case SulDownloader::suldownloaderdata::WarehouseStatus::DOWNLOADFAILED:
                event.MessageNumber = EventMessageNumber::DOWNLOADFAILED;
                buildMessagesFromReport(&event.Messages, report);
                break;
            case SulDownloader::suldownloaderdata::WarehouseStatus::INSTALLFAILED:
                event.MessageNumber = EventMessageNumber::INSTALLFAILED;
                buildMessagesFromReport(&event.Messages, report);
                break;
            case SulDownloader::suldownloaderdata::WarehouseStatus::UNSPECIFIED:
                event.MessageNumber = EventMessageNumber::INSTALLCAUGHTERROR;
                event.Messages.emplace_back("", report.getDescription());
                break;
            case SulDownloader::suldownloaderdata::WarehouseStatus::PACKAGESOURCEMISSING:
                handlePackageSourceMissing(&event, report);
                break;
            default:
                event.MessageNumber = EventMessageNumber::INSTALLCAUGHTERROR;
                event.Messages.emplace_back("", report.getDescription());
                break;
        }

        // if any product has changed the event is relevant to send.
        for( const auto & product: report.getProducts())
        {
            switch (product.productStatus)
            {
                case SulDownloader::suldownloaderdata::ProductReport::ProductStatus::Upgraded:
                case SulDownloader::suldownloaderdata::ProductReport::ProductStatus::Uninstalled:
                        event.IsRelevantToSend = true;
                        break;
                default:
                    break;
            }
        }

        event.UpdateSource = report.getSourceURL();

        return event;
    }

    UpdateStatus extractStatusFromSingleReport(
        const SulDownloader::suldownloaderdata::DownloadReport& report,
        const UpdateEvent& event)
    {
        UpdateStatus status;
        status.LastBootTime = TimeUtils::getBootTime();
        status.LastStartTime = report.getStartTime();
        status.LastFinishdTime = report.getFinishedTime();
        status.LastResult = event.MessageNumber;

        for (auto& product : report.getProducts())
        {
            if ( product.productStatus != SulDownloader::suldownloaderdata::ProductReport::ProductStatus::Uninstalled)
            {
                status.Products.emplace_back(product.rigidName, product.name, product.downloadedVersion);
            }
        }
        return status;
    }

} // namespace

namespace UpdateSchedulerImpl
{
    namespace configModule
    {
        ReportCollectionResult DownloadReportsAnalyser::processReports(
            const std::vector<SulDownloader::suldownloaderdata::DownloadReport>& reportCollection)
        {
            if (reportCollection.empty())
            {
                return ReportCollectionResult();
            }

            const SulDownloader::suldownloaderdata::DownloadReport& lastReport =
                reportCollection.at(reportCollection.size() - 1);

            if (lastReport.getStatus() == SulDownloader::suldownloaderdata::WarehouseStatus::SUCCESS)
            {
                return handleSuccessReports(reportCollection);
            }
            else
            {
                return handleFailureReports(reportCollection);
            }
        }

        ReportAndFiles DownloadReportsAnalyser::processReports()
        {
            std::vector<std::string> listFiles = Common::FileSystem::fileSystem()->listFiles(
                Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderReportPath());
            std::string configFile =
                Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderConfigFilePath();

            struct FileAndDownloadReport
            {
                std::string filepath;
                SulDownloader::suldownloaderdata::DownloadReport report;
                std::string sortKey;
            };

            std::vector<FileAndDownloadReport> reportCollection;

            for (auto& filepath : listFiles)
            {
                try
                {
                    if (filepath == configFile)
                    {
                        continue;
                    }
                    std::string content = Common::FileSystem::fileSystem()->readFile(filepath);
                    SulDownloader::suldownloaderdata::DownloadReport fileReport =
                        SulDownloader::suldownloaderdata::DownloadReport::toReport(content);
                    reportCollection.push_back(
                        FileAndDownloadReport{ filepath, fileReport, fileReport.getStartTime() });
                }
                catch (std::exception& ex)
                {
                    LOGERROR("Failed to process file: " << filepath);
                    LOGERROR(ex.what());
                }
            }
            LOGSUPPORT("Process " << reportCollection.size() << " suldownloader reports");
            std::sort(
                reportCollection.begin(),
                reportCollection.end(),
                [](const FileAndDownloadReport& lhs, const FileAndDownloadReport& rhs) {
                    return lhs.sortKey < rhs.sortKey;
                });

            for (auto& entry : reportCollection)
            {
                LOGSUPPORT("Sorted Listed files: " << entry.filepath << " key: " << entry.sortKey);
            }

            std::vector<std::string> sortedFilePaths;
            std::vector<SulDownloader::suldownloaderdata::DownloadReport> downloaderReports;

            for (auto& reportEntry : reportCollection)
            {
                downloaderReports.push_back(reportEntry.report);
                sortedFilePaths.push_back(reportEntry.filepath);
            }

            ReportAndFiles reportAndFiles{ .reportCollectionResult = processReports(downloaderReports),
                                           .sortedFilePaths = std::move(sortedFilePaths) };

            for (size_t i = 0; i < downloaderReports.size(); i++)
            {
                if (reportAndFiles.reportCollectionResult.IndicesOfSignificantReports[i] ==
                    ReportCollectionResult::SignificantReportMark::RedundantReport)
                {
                    LOGSUPPORT("Report " << reportAndFiles.sortedFilePaths[i] << " marked to be removed");
                }
            }

            return reportAndFiles;
        }

        /**
         * For success report there are the following cases:
         *   - current one (last one): has upgrade
         *   - one of the previous ones has upgrade.
         *   - None has upgrade.
         * @param reportCollection
         * @return
         */
        ReportCollectionResult DownloadReportsAnalyser::handleSuccessReports(
            const std::vector<SulDownloader::suldownloaderdata::DownloadReport>& reportCollection)
        {
            int lastIndex = reportCollection.size() - 1;
            assert(lastIndex >= 0);
            int indexOfLastUpgrade = lastUpgrade(reportCollection);
            ReportCollectionResult collectionResult;

            const SulDownloader::suldownloaderdata::DownloadReport& lastReport = reportCollection.at(lastIndex);

            collectionResult.SchedulerEvent = extractEventFromSingleReport(lastReport);

            // the status produced does not handle the following members
            // LastSyncTime
            // LastInstallStartedTime;
            // FirstFailedTime;
            collectionResult.SchedulerStatus =
                extractStatusFromSingleReport(lastReport, collectionResult.SchedulerEvent);

            // given that is a success, it has synchronized successfully
            collectionResult.SchedulerStatus.LastSyncTime = lastReport.getSyncTime();
            // success does not report FirstFailedTime
            collectionResult.SchedulerStatus.FirstFailedTime.clear();

            collectionResult.IndicesOfSignificantReports = std::vector<ReportCollectionResult::SignificantReportMark>(
                reportCollection.size(), ReportCollectionResult::SignificantReportMark::RedundantReport);
            collectionResult.IndicesOfSignificantReports.at(lastIndex) =
                ReportCollectionResult::SignificantReportMark::MustKeepReport;

            if (indexOfLastUpgrade != -1)
            {
                // the latest upgrade holds the LastInstallStartedTime
                collectionResult.SchedulerStatus.LastInstallStartedTime =
                    reportCollection.at(indexOfLastUpgrade).getStartTime();
            }

            // cover the following cases: only one element, the last element has upgrade.
            if (lastIndex == 0 || indexOfLastUpgrade == lastIndex)
            {
                collectionResult.SchedulerEvent.IsRelevantToSend = true;
                return collectionResult;
            }
            if (indexOfLastUpgrade != -1)
            {
                collectionResult.SchedulerStatus.LastInstallStartedTime =
                    reportCollection.at(indexOfLastUpgrade).getStartTime();

                collectionResult.IndicesOfSignificantReports.at(indexOfLastUpgrade) =
                    ReportCollectionResult::SignificantReportMark::MustKeepReport;
            }

            // is the event relevant?
            // there is at least two elements.
            int previousIndex = lastIndex - 1;
            // if previous one was an error, send event, otherwise do not send.
            collectionResult.SchedulerEvent.IsRelevantToSend |=
                reportCollection.at(previousIndex).getStatus() !=
                SulDownloader::suldownloaderdata::WarehouseStatus::SUCCESS;

            // if previous one had source url different from current, send event
            collectionResult.SchedulerEvent.IsRelevantToSend |=
                reportCollection.at(previousIndex).getSourceURL() != collectionResult.SchedulerEvent.UpdateSource;



            return collectionResult;
        }

        ReportCollectionResult DownloadReportsAnalyser::handleFailureReports(
            const std::vector<SulDownloader::suldownloaderdata::DownloadReport>& reportCollection)
        {
            assert(!reportCollection.empty());
            size_t lastIndex = reportCollection.size() - 1;

            DownloadReportVectorDifferenceType indexOfLastGoodSync = lastGoodSync(reportCollection);
            DownloadReportVectorDifferenceType indexOfLastUpgrade = lastUpgrade(reportCollection);
            ReportCollectionResult collectionResult;

            const auto& lastReport = reportCollection.at(lastIndex);

            collectionResult.SchedulerEvent = extractEventFromSingleReport(lastReport);

            // the status produced does not handle the following members
            // LastSyncTime
            // LastInstallStartedTime;
            // FirstFailedTime;
            collectionResult.SchedulerStatus =
                extractStatusFromSingleReport(lastReport, collectionResult.SchedulerEvent);

            collectionResult.IndicesOfSignificantReports = std::vector<ReportCollectionResult::SignificantReportMark>(
                reportCollection.size(), ReportCollectionResult::SignificantReportMark::RedundantReport);
            collectionResult.IndicesOfSignificantReports.at(lastIndex) =
                ReportCollectionResult::SignificantReportMark::MustKeepReport;

            if (indexOfLastUpgrade != -1)
            {
                assert(indexOfLastUpgrade >= 0);
                // the latest upgrade holds the LastInstallStartedTime
                collectionResult.SchedulerStatus.LastInstallStartedTime =
                    reportCollection.at(indexOfLastUpgrade).getStartTime();
                collectionResult.IndicesOfSignificantReports[indexOfLastUpgrade] =
                    ReportCollectionResult::SignificantReportMark::MustKeepReport;
            }

            if (indexOfLastGoodSync != -1)
            {
                collectionResult.SchedulerStatus.LastSyncTime = reportCollection.at(indexOfLastGoodSync).getSyncTime();
                // indexOfLastGoodSync + 1 must exists and it must be the first time synchronization failed.
                collectionResult.SchedulerStatus.FirstFailedTime =
                    reportCollection.at(indexOfLastGoodSync + 1).getStartTime();

                collectionResult.IndicesOfSignificantReports[indexOfLastGoodSync] =
                    ReportCollectionResult::SignificantReportMark::MustKeepReport;
                collectionResult.IndicesOfSignificantReports[indexOfLastGoodSync + 1] =
                    ReportCollectionResult::SignificantReportMark::MustKeepReport;
            }
            else
            {
                collectionResult.SchedulerStatus.FirstFailedTime = reportCollection.at(0).getStartTime();
            }

            if (reportCollection.size() == 1)
            {
                collectionResult.SchedulerEvent.IsRelevantToSend = true;
                return collectionResult;
            }

            UpdateEvent previousEvent = extractEventFromSingleReport(reportCollection.at(lastIndex - 1));

            collectionResult.SchedulerEvent.IsRelevantToSend =
                eventsAreDifferent(collectionResult.SchedulerEvent, previousEvent);

            // if the current report does not report products, we still need to list them, but we can do it only if
            // there is at least one LastGoodSync
            if (collectionResult.SchedulerStatus.Products.empty() && indexOfLastGoodSync != -1)
            {
                auto statusWithProducts =
                    extractStatusFromSingleReport(reportCollection.at(indexOfLastGoodSync), previousEvent);
                collectionResult.SchedulerStatus.Products = statusWithProducts.Products;
            }

            return collectionResult;
        }

        bool DownloadReportsAnalyser::hasUpgrade(const DownloadReport& report)
        {
            if (report.getStatus() == SulDownloader::suldownloaderdata::WarehouseStatus::SUCCESS)
            {
                for (auto& product : report.getProducts())
                {
                    if (product.productStatus ==
                        SulDownloader::suldownloaderdata::ProductReport::ProductStatus::Upgraded)
                    {
                        return true;
                    }
                }
            }
            return false;
        }

        DownloadReportsAnalyser::DownloadReportVectorDifferenceType DownloadReportsAnalyser::lastUpgrade(
            const DownloadReportVector& reports)
        {
            auto rind = std::find_if(
                reports.rbegin(), reports.rend(), [](const SulDownloader::suldownloaderdata::DownloadReport& report) {
                    return hasUpgrade(report);
                });
            return std::distance(reports.begin(), rind.base()) - 1;
        }

        DownloadReportsAnalyser::DownloadReportVectorDifferenceType DownloadReportsAnalyser::lastGoodSync(
            const DownloadReportVector& reports)
        {
            auto rind = std::find_if(reports.rbegin(), reports.rend(), [](const DownloadReport& report) {
                return report.getStatus() == SulDownloader::suldownloaderdata::WarehouseStatus::SUCCESS;
            });
            return std::distance(reports.begin(), rind.base()) - 1;
        }

        bool DownloadReportsAnalyser::eventsAreDifferent(const UpdateEvent& lhs, const UpdateEvent& rhs)
        {
            if (lhs.MessageNumber != rhs.MessageNumber || lhs.UpdateSource != rhs.UpdateSource)
            {
                return true;
            }

            if (lhs.Messages.size() != rhs.Messages.size())
            {
                return true;
            }
            for (size_t i = 0; i < lhs.Messages.size(); i++)
            {
                if (lhs.Messages[i].PackageName != rhs.Messages[i].PackageName)
                {
                    return true;
                }
                if (lhs.Messages[i].ErrorDetails != rhs.Messages[i].ErrorDetails)
                {
                    return true;
                }
            }
            return false;
        }
    } // namespace configModule
} // namespace UpdateSchedulerImpl