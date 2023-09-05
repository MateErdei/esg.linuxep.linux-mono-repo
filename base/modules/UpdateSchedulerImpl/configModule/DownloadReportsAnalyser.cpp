// Copyright 2018-2023 Sophos Limited. All rights reserved.
#include "DownloadReportsAnalyser.h"

#include "UpdateStatus.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/UpdateUtilities/DownloadReports.h"
#include "Common/UtilityImpl/StringUtils.h"
#include "UpdateSchedulerImpl/common/Logger.h"

#include <algorithm>
#include <cassert>

using namespace Common::UtilityImpl;
using namespace Common::DownloadReport;

namespace
{
    using MessageInsert = UpdateSchedulerImpl::configModule::MessageInsert;
    using UpdateEvent = UpdateSchedulerImpl::configModule::UpdateEvent;
    using EventMessageNumber = UpdateSchedulerImpl::EventMessageNumber;
    using UpdateStatus = UpdateSchedulerImpl::configModule::UpdateStatus;

    void buildMessagesInsertFromDownloadReport(std::vector<MessageInsert>* messages, const DownloadReport& report)
    {
        for (const auto& product : report.getProducts())
        {
            if (!product.errorDescription.empty())
            {
                messages->emplace_back(product.name, product.errorDescription);
            }
        }
        // If we haven't found any product errors, then try the report level error instead
        if (messages->empty() && !report.getDescription().empty())
        {
            messages->emplace_back("SSPL", report.getDescription());
        }
    }

    void buildMessagesFromReport(std::vector<MessageInsert>* messages, const DownloadReport& report)
    {
        buildMessagesInsertFromDownloadReport(messages, report);
        // if no specific error associated with a product is present, report the general error.
        if (messages->empty())
        {
            messages->emplace_back("", report.getDescription());
        }
    }

    void handlePackageSourceMissing(UpdateEvent* event, const DownloadReport& report)
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

    UpdateEvent extractEventFromSingleReport(const DownloadReport& report)
    {
        UpdateEvent event;
        switch (report.getStatus())
        {
            case RepositoryStatus::SUCCESS:
                event.MessageNumber = EventMessageNumber::SUCCESS;
                break;
            case RepositoryStatus::CONNECTIONERROR:
                event.MessageNumber = EventMessageNumber::CONNECTIONERROR;
                // no package name, just the error details
                event.Messages.emplace_back("", report.getDescription());
                break;
            case RepositoryStatus::DOWNLOADFAILED:
                event.MessageNumber = EventMessageNumber::DOWNLOADFAILED;
                buildMessagesFromReport(&event.Messages, report);
                break;
            case RepositoryStatus::INSTALLFAILED:
                event.MessageNumber = EventMessageNumber::INSTALLFAILED;
                buildMessagesFromReport(&event.Messages, report);
                break;
            case RepositoryStatus::UNSPECIFIED:
                event.MessageNumber = EventMessageNumber::INSTALLCAUGHTERROR;
                event.Messages.emplace_back("", report.getDescription());
                break;
            case RepositoryStatus::PACKAGESOURCEMISSING:
                handlePackageSourceMissing(&event, report);
                break;
            default:
                event.MessageNumber = EventMessageNumber::INSTALLCAUGHTERROR;
                event.Messages.emplace_back("", report.getDescription());
                break;
        }

        // if any product has changed the event is relevant to send.
        for (const auto& product : report.getProducts())
        {
            switch (product.productStatus)
            {
                case ProductReport::ProductStatus::Upgraded:
                case ProductReport::ProductStatus::Uninstalled:
                    event.IsRelevantToSend = true;
                    break;
                default:
                    break;
            }
        }

        event.UpdateSource = report.getSourceURL();

        return event;
    }

    UpdateStatus extractStatusFromSingleReport(const DownloadReport& report, const UpdateEvent& event)
    {
        UpdateStatus status;
        status.LastBootTime = TimeUtils::getBootTime();
        status.LastStartTime = report.getStartTime();
        status.LastFinishdTime = report.getFinishedTime();
        status.LastResult = event.MessageNumber;
        status.LastUpdateWasSupplementOnly = report.isSupplementOnlyUpdate();

        for (const auto& product : report.getProducts())
        {
            if (product.productStatus != ProductReport::ProductStatus::Uninstalled)
            {
                status.Subscriptions.emplace_back(product.rigidName, product.name, product.downloadedVersion);
            }
        }

        for (const auto& whComponent : report.getRepositoryComponents())
        {
            status.Products.emplace_back(
                whComponent.m_rigidName,
                whComponent.m_productName,
                whComponent.m_version,
                whComponent.m_installedVersion);
        }
        return status;
    }

    using UpdateSchedulerImpl::configModule::DownloadReportsAnalyser;

    DownloadReportsAnalyser::DownloadReportVectorDifferenceType lastProductUpdateCheck(
        const DownloadReportsAnalyser::DownloadReportVector& reports)
    {
        auto rind = std::find_if(
            reports.rbegin(),
            reports.rend(),
            [](const DownloadReportsAnalyser::DownloadReport& report)
            { return report.isSuccesfulProductUpdateCheck(); });
        return std::distance(reports.begin(), rind.base()) - 1;
    }

} // namespace

namespace UpdateSchedulerImpl::configModule
{
    ReportCollectionResult DownloadReportsAnalyser::processReports(const DownloadReportVector& reportCollection)
    {
        if (reportCollection.empty())
        {
            return ReportCollectionResult();
        }

        const DownloadReport& lastReport = reportCollection.at(reportCollection.size() - 1);
        ReportCollectionResult collectionResult;
        if (lastReport.getStatus() == RepositoryStatus::SUCCESS)
        {
            collectionResult = handleSuccessReports(reportCollection);
        }
        else
        {
            collectionResult = handleFailureReports(reportCollection);
        }

        // special case is required for the status to report the list of warehousecomponents because
        // it is necessary to look for previous report when no product is listed.
        if (collectionResult.SchedulerStatus.Products.empty())
        {
            // reverse iteration to find the latest report with non empty products
            // skipping the latest one that has already been checked
            for (int i = static_cast<int>(reportCollection.size()) - 2; i >= 0; i--)
            {
                UpdateStatus lastStatus =
                    extractStatusFromSingleReport(reportCollection[i], collectionResult.SchedulerEvent);
                if (!lastStatus.Products.empty())
                {
                    collectionResult.SchedulerStatus.Products = lastStatus.Products;
                    break;
                }
            }
        }
        return collectionResult;
    }

    DownloadReportsAnalyser::FileAndDownloadReportVector DownloadReportsAnalyser::readSortedReports()
    {
        auto listOfReportFiles = Common::UpdateUtilities::listOfAllPreviousReports(
            Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderReportPath());

        std::vector<FileAndDownloadReport> reportCollection;
        reportCollection.reserve(listOfReportFiles.size());

        const std::string processedReportPath =
            Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderProcessedReportPath();

        for (const auto& filepath : listOfReportFiles)
        {
            try
            {
                std::string content = Common::FileSystem::fileSystem()->readFile(filepath);
                DownloadReport fileReport = DownloadReport::toReport(content);

                // check to see if report has been processed
                if (Common::FileSystem::fileSystem()->isFile(
                        Common::FileSystem::join(processedReportPath, Common::FileSystem::basename(filepath))))
                {
                    fileReport.setProcessedReport(true);
                }

                reportCollection.push_back(FileAndDownloadReport{ filepath, fileReport, fileReport.getStartTime() });
            }
            catch (const std::exception& ex)
            {
                LOGERROR("Failed to process file: " << filepath << ": " << ex.what());
            }
        }
        std::sort(
            reportCollection.begin(),
            reportCollection.end(),
            [](const FileAndDownloadReport& lhs, const FileAndDownloadReport& rhs)
            { return lhs.sortKey < rhs.sortKey; });

        return reportCollection;
    }

    ReportAndFiles DownloadReportsAnalyser::processReports()
    {
        auto reportCollection = readSortedReports();

        LOGSUPPORT("Process " << reportCollection.size() << " suldownloader reports");
        for (auto& entry : reportCollection)
        {
            LOGSUPPORT("Sorted Listed files: " << entry.filepath << " key: " << entry.sortKey);
        }

        std::vector<std::string> sortedFilePaths;
        std::vector<DownloadReport> downloaderReports;

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
    ReportCollectionResult DownloadReportsAnalyser::handleSuccessReports(const DownloadReportVector& reportCollection)
    {
        assert(!reportCollection.empty());
        DownloadReportVectorSizeType lastIndex = reportCollection.size() - 1;
        int indexOfLastUpgrade = lastUpgrade(reportCollection); // must be signed as might be -1
        ReportCollectionResult collectionResult;

        const DownloadReport& lastReport = reportCollection.at(lastIndex);

        collectionResult.SchedulerEvent = extractEventFromSingleReport(lastReport);

        // the status produced does not handle the following members
        // LastSyncTime
        // LastInstallStartedTime;
        // FirstFailedTime;
        collectionResult.SchedulerStatus = extractStatusFromSingleReport(lastReport, collectionResult.SchedulerEvent);

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

        // Find the last product update, and mark that one to keep
        int indexOfProductUpdateCheck = lastProductUpdateCheck(reportCollection); // must be signed to allow -1
        if (indexOfProductUpdateCheck != -1)
        {
            collectionResult.IndicesOfSignificantReports.at(indexOfProductUpdateCheck) =
                ReportCollectionResult::SignificantReportMark::MustKeepReport;
        }

        // cover the following cases: only one element, the last element has upgrade.
        if (lastIndex == 0 || indexOfLastUpgrade == static_cast<int>(lastIndex))
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
        assert(lastIndex > 0);
        DownloadReportVectorSizeType previousIndex = lastIndex - 1;
        // if previous one was an error, send event, otherwise do not send.
        collectionResult.SchedulerEvent.IsRelevantToSend |=
            reportCollection.at(previousIndex).getStatus() != RepositoryStatus::SUCCESS;

        // Run through the report collection, and check if there is an old un-processed report containing
        // an Upgrade or uninstalled state., if there is set IsRelevantToSend to true to force sending an
        // Update Status Event.
        for (const auto& report : reportCollection)
        {
            if (report.isProcessedReport())
            {
                continue;
            }

            for (const auto& product : report.getProducts())
            {
                if (product.productStatus == ProductReport::ProductStatus::Upgraded ||
                    product.productStatus == ProductReport::ProductStatus::Uninstalled)
                {
                    collectionResult.SchedulerEvent.IsRelevantToSend = true;
                }
            }
        }

        // if previous one had source url different from current, send event
        collectionResult.SchedulerEvent.IsRelevantToSend |=
            reportCollection.at(previousIndex).getSourceURL() != collectionResult.SchedulerEvent.UpdateSource;

        return collectionResult;
    }

    ReportCollectionResult DownloadReportsAnalyser::handleFailureReports(const DownloadReportVector& reportCollection)
    {
        assert(!reportCollection.empty());
        DownloadReportVectorSizeType lastIndex = reportCollection.size() - 1;

        DownloadReportVectorDifferenceType indexOfLastGoodSync = lastGoodSync(reportCollection);
        DownloadReportVectorDifferenceType indexOfLastUpgrade = lastUpgrade(reportCollection);
        int indexOfProductUpdateCheck = lastProductUpdateCheck(reportCollection); // must be signed to allow -1
        ReportCollectionResult collectionResult;

        const auto& lastReport = reportCollection.at(lastIndex);

        collectionResult.SchedulerEvent = extractEventFromSingleReport(lastReport);

        // the status produced does not handle the following members
        // LastSyncTime
        // LastInstallStartedTime;
        // FirstFailedTime;
        collectionResult.SchedulerStatus = extractStatusFromSingleReport(lastReport, collectionResult.SchedulerEvent);

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

        if (indexOfProductUpdateCheck != -1)
        {
            collectionResult.IndicesOfSignificantReports.at(indexOfProductUpdateCheck) =
                ReportCollectionResult::SignificantReportMark::MustKeepReport;
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
        if (collectionResult.SchedulerStatus.Subscriptions.empty() && indexOfLastGoodSync != -1)
        {
            auto statusWithProducts =
                extractStatusFromSingleReport(reportCollection.at(indexOfLastGoodSync), previousEvent);
            collectionResult.SchedulerStatus.Subscriptions = statusWithProducts.Subscriptions;
        }

        return collectionResult;
    }

    bool DownloadReportsAnalyser::hasUpgrade(const DownloadReport& report)
    {
        if (report.getStatus() == RepositoryStatus::SUCCESS)
        {
            for (const auto& product : report.getProducts())
            {
                if (product.productStatus == ProductReport::ProductStatus::Upgraded)
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
            reports.rbegin(), reports.rend(), [](const DownloadReport& report) { return hasUpgrade(report); });
        return std::distance(reports.begin(), rind.base()) - 1;
    }

    DownloadReportsAnalyser::DownloadReportVectorDifferenceType DownloadReportsAnalyser::lastGoodSync(
        const DownloadReportVector& reports)
    {
        auto rind = std::find_if(
            reports.rbegin(),
            reports.rend(),
            [](const DownloadReport& report) { return report.getStatus() == RepositoryStatus::SUCCESS; });
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
    time_t DownloadReportsAnalyser::getLastProductUpdateCheck()
    {
        auto reportCollection = readSortedReports();

        if (reportCollection.empty())
        {
            return 0;
        }
        std::reverse(reportCollection.begin(), reportCollection.end());

        for (const auto& reportHolder : reportCollection)
        {
            /*
             * Only consider successful update-checks
             * If updates are failing, then try product-update-check every time.
             */
            if (reportHolder.report.isSuccesfulProductUpdateCheck())
            {
                return convertStringTimeToTimet(reportHolder.report.getStartTime());
            }
        }

        return 0; // oldest possible time
    }

    time_t DownloadReportsAnalyser::convertStringTimeToTimet(const std::string& timeStr)
    {
        return Common::UtilityImpl::TimeUtils::toTime(timeStr);
    }
} // namespace UpdateSchedulerImpl::configModule