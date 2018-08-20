/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "UpdateStatus.h"
#include <Common/UtilityImpl/StringUtils.h>
#include <Common/UtilityImpl/TimeUtils.h>

using StringUtils = Common::UtilityImpl::StringUtils;
namespace {
    using ProductStatus = UpdateSchedulerImpl::configModule::ProductStatus;
    std::string subscriptionTemplate{R"sophos(
        <subscription rigidName="@@rigidName@@" version="@@version@@" displayVersion="@@displayversion@@" />)sophos"};

    std::string firstFailedTemplate{R"sophos(
        <firstFailedTime>@@failedTime@@</firstFailedTime>)sophos"};

    std::string getsubscriptionsElement(const std::vector<ProductStatus>& products)
    {
        std::string subscriptions;
        for( auto & product: products)
        {
            std::string subscriptionLine = StringUtils::orderedStringReplace(subscriptionTemplate, {
                {"@@rigidName@@", product.RigidName},
                {"@@version@@", product.DownloadedVersion},
                {"@@displayversion@@", product.DownloadedVersion}
            });
            subscriptions += subscriptionLine;
        }
        return subscriptions;

    }

    std::string getFirstFailedElement(const std::string & failedTime)
    {
        if ( failedTime.empty())
        {
            return failedTime;
        }
        return StringUtils::orderedStringReplace(firstFailedTemplate, {{"@@failedTime@@", failedTime}});
    }

    std::string statusTemplate{R"sophos(<?xml version="1.0" encoding="utf-8" ?>
<status xmlns="com.sophos\mansys\status" type="sau">
    <CompRes xmlns="com.sophos\msys\csc" Res="Same" RevID="@@revid@@" policyType="1" />
    <autoUpdate xmlns="http://www.sophos.com/xml/mansys/AutoUpdateStatus.xsd" version="@@version@@">
        <lastBootTime>@@boottime@@</lastBootTime>
        <lastStartedTime>@@lastStartedTime@@</lastStartedTime>
        <lastSyncTime>@@lastSyncTime@@</lastSyncTime>
        <lastInstallStartedTime>@@lastInstallStartedTime@@</lastInstallStartedTime>
        <lastFinishedTime>@@lastFinishedTime@@</lastFinishedTime>@@firstfailedTimeElement@@
        <lastResult>@@lastResult@@</lastResult>
        <endpoint id="@@endpointid@@" />
    </autoUpdate>
    <subscriptions>@@subscriptionsElement@@
    </subscriptions>
</status>)sophos"};

}

namespace UpdateSchedulerImpl
{
    namespace configModule
    {
        std::string SerializeUpdateStatus(const UpdateSchedulerImpl::configModule::UpdateStatus& status,
                                          const std::string& revID,
                                          const std::string& versionId,
                                          const Common::UtilityImpl::IFormattedTime& iFormattedTime)
        {

            std::string subscriptionsElement = getsubscriptionsElement(status.Products);
            std::string firstFailedElement = getFirstFailedElement(status.FirstFailedTime);
            std::string bootTime = iFormattedTime.bootTime();
            return StringUtils::orderedStringReplace(statusTemplate, {
                                                             {  "@@revid@@"                 , revID}
                                                             , {"@@version@@"               , versionId}
                                                             , {"@@boottime@@"              , bootTime}
                                                             , {"@@lastStartedTime@@"       , status.LastStartTime}
                                                             , {"@@lastSyncTime@@"          , status.LastSyncTime}
                                                             , {"@@lastInstallStartedTime@@", status.LastInstallStartedTime}
                                                             , {"@@lastFinishedTime@@"      , status.LastFinishdTime}
                                                             , {"@@firstfailedTimeElement@@", firstFailedElement}
                                                             , {"@@lastResult@@"            , std::to_string(status.LastResult)}
                                                             , {"@@endpointid@@"            , "NotImplemented"}
                                                             , //FIXME LINUXEP-6474
                                                             {  "@@subscriptionsElement@@"  , subscriptionsElement}
                                                     }
            );
        }

    }
}