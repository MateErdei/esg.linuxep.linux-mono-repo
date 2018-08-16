/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "DownloadReportsAnalyser.h"

#include <Common/UtilityImpl/StringUtils.h>
#include <Common/UtilityImpl/TimeUtils.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <algorithm>
#include <cassert>

using namespace Common::UtilityImpl;
namespace
{
    enum EventMessageNumber{SUCCESS=0, INSTALLFAILED=103, INSTALLCAUGHTERROR=106, DOWNLOADFAILED=107,
            UPDATECANCELLED=108, RESTARTEDNEEDED=109, UPDATESOURCEMISSING=110,
            PACKAGESOURCEMISSING=111, CONNECTIONERROR=112, PACKAGESSOURCESMISSING=113  };





    void buildMessagesInsertFromDownloadReport( std::vector<UpdateScheduler::MessageInsert> * messages, const SulDownloader::DownloadReport & report )
    {
        for ( auto & product : report.getProducts())
        {
            if (!product.errorDescription.empty())
            {
                messages->emplace_back(product.name, product.errorDescription);
            }
        }
    }


    void buildMessagesFromReport( std::vector<UpdateScheduler::MessageInsert> * messages, const SulDownloader::DownloadReport & report)
    {
        buildMessagesInsertFromDownloadReport(messages, report);
        // if no specific error associated with a product is present, report the general error.
        if (messages->empty())
        {
            messages->emplace_back("", report.getDescription());
        }
    }

    void handlePackageSourceMissing(UpdateScheduler::UpdateEvent * event, const SulDownloader::DownloadReport & report)
    {
        std::vector<std::string> splittedEntries = Common::UtilityImpl::StringUtils::splitString(report.getDescription(), ";");
        if ( splittedEntries.empty())
        {
            event->MessageNumber = EventMessageNumber::PACKAGESOURCEMISSING;
            return;
        }
        if ( splittedEntries.size() == 1)
        {
            event->MessageNumber = EventMessageNumber::PACKAGESOURCEMISSING;
            event->Messages.emplace_back(splittedEntries[0],"");
        }
        else
        {
            event->MessageNumber = EventMessageNumber::PACKAGESSOURCESMISSING;
            for(auto & entry: splittedEntries)
            {
                event->Messages.emplace_back(entry, "");
            }
        }
    }

    UpdateScheduler::UpdateEvent extractEventFromSingleReport( const SulDownloader::DownloadReport & report)
    {
        UpdateScheduler::UpdateEvent event;
        switch (report.getStatus())
        {
            case SulDownloader::WarehouseStatus::SUCCESS:
                event.MessageNumber =  EventMessageNumber::SUCCESS;
                break;
            case SulDownloader::WarehouseStatus::CONNECTIONERROR:
                event.MessageNumber =  EventMessageNumber::CONNECTIONERROR;
                // no package name, just the error details
                event.Messages.emplace_back("", report.getDescription());
                break;
            case SulDownloader::WarehouseStatus::DOWNLOADFAILED:
                event.MessageNumber = EventMessageNumber::DOWNLOADFAILED;
                buildMessagesFromReport( &event.Messages, report);
                break;
            case SulDownloader::WarehouseStatus::INSTALLFAILED:
                event.MessageNumber = EventMessageNumber::INSTALLFAILED;
                buildMessagesFromReport( & event.Messages, report);
                break;
            case SulDownloader::WarehouseStatus::UNSPECIFIED:
                event.MessageNumber = EventMessageNumber::INSTALLCAUGHTERROR;
                event.Messages.emplace_back("", report.getDescription());
                break;
            case SulDownloader::WarehouseStatus::PACKAGESOURCEMISSING:
                handlePackageSourceMissing(&event, report);
                break;
            default:
                event.MessageNumber =  EventMessageNumber::INSTALLCAUGHTERROR;
                event.Messages.emplace_back("", report.getDescription());
                break;
        }

        event.UpdateSource = report.getSourceURL();

        return event;
    }

    UpdateScheduler::UpdateStatus extractStatusFromSingleReport( const SulDownloader::DownloadReport & report, const UpdateScheduler::UpdateEvent & event)
    {
        UpdateScheduler::UpdateStatus status;
        status.LastBootTime = TimeUtils::getBootTime();
        status.LastStartTime = report.getStartTime();
        status.LastFinishdTime = report.getFinishedTime();
        status.LastResult = event.MessageNumber;


        for ( auto & product: report.getProducts())
        {
            status.Products.emplace_back(product.rigidName, product.name, product.downloadedVersion, product.installedVersion);
        }
        return status;

    }




}



namespace UpdateScheduler
{


    ReportCollectionResult DownloadReportsAnalyser::processReports(const std::vector<SulDownloader::DownloadReport> &reportCollection)
    {

        if ( reportCollection.empty())
        {
            return ReportCollectionResult();
        }

        const SulDownloader::DownloadReport & lastReport = reportCollection.at(reportCollection.size()-1);

        if ( lastReport.getStatus() == SulDownloader::WarehouseStatus::SUCCESS)
        {
            return handleSuccessReports( reportCollection);
        }
        else
        {
            return handleFailureReports( reportCollection);
        }
    }

    ReportAndFiles DownloadReportsAnalyser::processReports()
    {
        std::vector<std::string> listFiles = Common::FileSystem::fileSystem()->listFiles(Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderReportPath());

        struct FileAndDonloadReport
        {
            std::string filepath;
            SulDownloader::DownloadReport report;
            std::string sortKey;
        };

        std::vector<FileAndDonloadReport> reportCollection;

        for( auto & filepath: listFiles)
        {
            try
            {
                std::string content = Common::FileSystem::fileSystem()->readFile( filepath );
                SulDownloader::DownloadReport fileReport = SulDownloader::DownloadReport::toReport(content);
                reportCollection.push_back(FileAndDonloadReport{filepath, fileReport, fileReport.getStartTime()});
            }catch (std::exception & ex)
            {
                // todo log error
            }
        }

        std::sort(reportCollection.begin(), reportCollection.end(), [](const FileAndDonloadReport & lhs, const FileAndDonloadReport & rhs){return lhs.sortKey < rhs.sortKey; });


        ReportAndFiles reportAndFiles;
        std::vector<SulDownloader::DownloadReport> downloaderReports;
        for( auto & reportEntry: reportCollection)
        {
            downloaderReports.push_back(reportEntry.report);
            reportAndFiles.sortedFilePaths.push_back(reportEntry.filepath);
        }

        reportAndFiles.reportCollectionResult = processReports( downloaderReports);

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
    ReportCollectionResult DownloadReportsAnalyser::handleSuccessReports(const std::vector<SulDownloader::DownloadReport> &reportCollection)
    {
        int lastIndex = reportCollection.size() -1;
        assert( lastIndex>= 0);
        int indexOfLastUpgrade = lastUpgrade(reportCollection);
        ReportCollectionResult collectionResult;

        const SulDownloader::DownloadReport & lastReport = reportCollection.at(lastIndex);

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



        collectionResult.IndicesOfSignificantReports = std::vector<ReportCollectionResult::SignificantReportMark>(reportCollection.size(), ReportCollectionResult::SignificantReportMark::RedundantReport);
        collectionResult.IndicesOfSignificantReports.at(lastIndex) = ReportCollectionResult::SignificantReportMark::MustKeepReport;

        if ( indexOfLastUpgrade != -1)
        {
            // the latest upgrade holds the LastInstallStartedTime
            collectionResult.SchedulerStatus.LastInstallStartedTime = reportCollection.at(indexOfLastUpgrade).getStartTime();
        }

        // cover the following cases: No upgrade, only one element, the last element has upgrade.
        if (indexOfLastUpgrade == -1 || indexOfLastUpgrade == lastIndex)
        {
            collectionResult.SchedulerEvent.IsRelevantToSend = true;
            return collectionResult;
        }

        // last one was not an upgrade

        collectionResult.SchedulerStatus.LastInstallStartedTime = reportCollection.at(indexOfLastUpgrade).getStartTime();

        collectionResult.IndicesOfSignificantReports.at(indexOfLastUpgrade) = ReportCollectionResult::SignificantReportMark::MustKeepReport;

        // is the event relevant?
        // there is at least two elements.
        int previousIndex = lastIndex -1;
        // if previous one was an error, send event, otherwise do not send.
        collectionResult.SchedulerEvent.IsRelevantToSend = reportCollection.at(previousIndex).getStatus() != SulDownloader::WarehouseStatus::SUCCESS;


        return collectionResult;
    }


    ReportCollectionResult DownloadReportsAnalyser::handleFailureReports(const std::vector<SulDownloader::DownloadReport> &reportCollection)
    {
        int lastIndex = reportCollection.size() -1;
        assert( lastIndex>= 0);
        int indexOfLastGoodSync = lastGoodSync(reportCollection);
        int indexOfLastUpgrade = lastUpgrade(reportCollection);
        ReportCollectionResult collectionResult;

        const SulDownloader::DownloadReport & lastReport = reportCollection.at(lastIndex);

        collectionResult.SchedulerEvent = extractEventFromSingleReport(lastReport);


        // the status produced does not handle the following members
        // LastSyncTime
        // LastInstallStartedTime;
        // FirstFailedTime;
        collectionResult.SchedulerStatus = extractStatusFromSingleReport(lastReport, collectionResult.SchedulerEvent);

        collectionResult.IndicesOfSignificantReports = std::vector<ReportCollectionResult::SignificantReportMark>(reportCollection.size(), ReportCollectionResult::SignificantReportMark::RedundantReport);
        collectionResult.IndicesOfSignificantReports.at(lastIndex) = ReportCollectionResult::SignificantReportMark::MustKeepReport;


        if ( indexOfLastUpgrade != -1)
        {
            // the latest upgrade holds the LastInstallStartedTime
            collectionResult.SchedulerStatus.LastInstallStartedTime = reportCollection.at(indexOfLastUpgrade).getStartTime();
            collectionResult.IndicesOfSignificantReports[indexOfLastUpgrade] = ReportCollectionResult::SignificantReportMark::MustKeepReport;
        }

        if( indexOfLastGoodSync != -1 )
        {

            collectionResult.SchedulerStatus.LastSyncTime = reportCollection.at(indexOfLastGoodSync).getSyncTime();
            // indexOfLastGoodSync + 1 must exists and it must be the first time synchronization failed.
            collectionResult.SchedulerStatus.FirstFailedTime = reportCollection.at(indexOfLastGoodSync+1).getStartTime();

            collectionResult.IndicesOfSignificantReports[indexOfLastGoodSync] = ReportCollectionResult::SignificantReportMark::MustKeepReport;
            collectionResult.IndicesOfSignificantReports[indexOfLastGoodSync+1] = ReportCollectionResult::SignificantReportMark::MustKeepReport;
        }
        else
        {
            collectionResult.SchedulerStatus.FirstFailedTime = reportCollection.at(0).getStartTime();
        }

        if( reportCollection.size() == 1)
        {
            collectionResult.SchedulerEvent.IsRelevantToSend = true;
            return collectionResult;
        }

        UpdateEvent previousEvent = extractEventFromSingleReport(reportCollection.at(lastIndex-1));

        collectionResult.SchedulerEvent.IsRelevantToSend = eventsAreDifferent(collectionResult.SchedulerEvent, previousEvent);


        // if the current report does not report products, we still need to list them, but we can do it only if there is at least one LastGoodSync
        if ( collectionResult.SchedulerStatus.Products.empty() &&  indexOfLastGoodSync != -1)
        {
            auto statusWithProducts = extractStatusFromSingleReport(reportCollection.at(indexOfLastGoodSync), previousEvent);
            collectionResult.SchedulerStatus.Products = statusWithProducts.Products;
        }


        return collectionResult;
    }

    bool DownloadReportsAnalyser::hasUpgrade(const SulDownloader::DownloadReport &report)
    {
        if ( report.getStatus() == SulDownloader::WarehouseStatus::SUCCESS)
        {
            for( auto & product: report.getProducts())
            {
                if ( product.productStatus == SulDownloader::ProductReport::ProductStatus::Upgraded)
                {
                    return true;
                }
            }
        }
        return false;
    }

    int DownloadReportsAnalyser::lastUpgrade(const std::vector<SulDownloader::DownloadReport> & reports)
    {
        auto rind = std::find_if( reports.rbegin(), reports.rend(),[](const SulDownloader::DownloadReport& report){return hasUpgrade(report);  } );
        return std::distance(reports.begin(), rind.base())-1;
    }

    int DownloadReportsAnalyser::lastGoodSync(const std::vector<SulDownloader::DownloadReport> & reports)
    {
        auto rind = std::find_if( reports.rbegin(), reports.rend(),[](const SulDownloader::DownloadReport& report){return report.getStatus() == SulDownloader::WarehouseStatus::SUCCESS;  } );
        return std::distance(reports.begin(), rind.base())-1;
    }

    bool DownloadReportsAnalyser::eventsAreDifferent(const UpdateEvent &lhs, const UpdateEvent &rhs)
    {
        if( lhs.MessageNumber != rhs.MessageNumber || lhs.UpdateSource != rhs.UpdateSource)
        {
            return true;
        }

        if ( lhs.Messages.size() != rhs.Messages.size())
        {
            return true;
        }
        for(size_t i = 0; i< lhs.Messages.size(); i++)
        {
            if( lhs.Messages[i].PackageName != rhs.Messages[i].PackageName)
            {
                return true;
            }
            if( lhs.Messages[i].ErrorDetails != rhs.Messages[i].ErrorDetails)
            {
                return true;
            }

        }
        return false;
    }
}