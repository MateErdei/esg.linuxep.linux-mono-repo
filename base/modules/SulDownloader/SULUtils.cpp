/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SULUtils.h"
#include "suldownloaderdata/Logger.h"
#include <iostream>
#include <vector>


namespace
{
    class SulStringResource
    {
    public:
        SulStringResource( char * cstring, SU_Handle session):
                m_str(cstring), m_session(session)
        {

        }
        SulStringResource& operator=(const SulStringResource & ) = delete;
        SulStringResource(const SulStringResource & ) = delete;
        ~SulStringResource()
        {
            if (m_str != nullptr)
            {
                SU_freeString(m_session, m_str);
            }

        }
        std::string str()
        {
            if (m_str== nullptr)
            {
                return "";
            }
            return m_str;
        }

    private:
        char * m_str;
        SU_Handle m_session;

    };

    SulDownloader::suldownloaderdata::WarehouseStatus fromSulCode( SU_Result sulCode)
    {
        switch (sulCode)
        {
            case SU_Result_OK:
            case SU_Result_continue:
                return SulDownloader::suldownloaderdata::WarehouseStatus::SUCCESS;
            case SU_Result_proxyAuthenticationFailure:
            case SU_Result_credentialsInvalid:
                return SulDownloader::suldownloaderdata::WarehouseStatus::CONNECTIONERROR;
            case SU_Result_cancelled:
            case SU_Result_corruptData:
            case SU_Result_sourceOutOfDate:
                return SulDownloader::suldownloaderdata::WarehouseStatus::DOWNLOADFAILED;
            case SU_Result_productNotFound:
            case SU_Result_productMissing:
            case SU_Result_noPackages:
            case SU_Result_fixedVersionMissing:
                return SulDownloader::suldownloaderdata::WarehouseStatus::PACKAGESOURCEMISSING;
            case SU_Result_invalid:
            case SU_Result_notSupported:
            case SU_Result_unspecifiedFailure:
            case SU_Result_notAttempted:
            case SU_Result_noMemory:
            case SU_Result_failed:
            case SU_Result_nullSuccess:
            default:
                return SulDownloader::suldownloaderdata::WarehouseStatus::UNSPECIFIED;
        }
    }

}

namespace  SulDownloader
{

    std::string SulGetErrorDetails( SU_Handle session )
    {
        SulStringResource s(SU_getErrorDetails(session),session);
        return s.str();
    }
    std::string SulGetLogEntry( SU_Handle session)
    {
        SulStringResource s(SU_getLogEntry(session), session);
        return s.str();
    }
    std::string SulQueryProductMetadata( SU_PHandle product, const std::string & attribute, SU_Int index)
    {
        SulStringResource s( SU_queryProductMetadata(product, attribute.c_str(), index), SU_getSession(product));
        return s.str();

    }

    bool SULUtils::isSuccess(SU_Result result)
    {
        return result == SU_Result_OK
               || result == SU_Result_nullSuccess
               || result == SU_Result_notAttempted;
    }

    void SULUtils::displayLogs(SU_Handle ses)
    {
        if ( !isSuccess(SU_getLastError(ses)))
        {
            LOGERROR("Error: " << SulGetErrorDetails(ses));
        }

        for ( auto & log : SulLogs(ses))
        {
            LOGINFO(log);
        }

    }


    std::vector<std::string> SULUtils::SulLogs(SU_Handle ses)
    {
        std::vector<std::string> logs;
        while (true)
        {
            std::string log = SulGetLogEntry(ses);
            if (log.empty())
            {
                break;
            }
            logs.emplace_back(log);

        }
        return logs;
    }

    std::pair<suldownloaderdata::WarehouseStatus, std::string> getSulCodeAndDescription(SU_Handle session)
    {
        std::pair<suldownloaderdata::WarehouseStatus, std::string> sulStatus(suldownloaderdata::WarehouseStatus::UNSPECIFIED, "");
        if ( session == nullptr)
        {
            return sulStatus;
        }
        auto result = SU_getLastError(session);
        if ( !SULUtils::isSuccess(result) )
        {
            sulStatus.first = fromSulCode(result);
            sulStatus.second = SulGetErrorDetails(session);
        }
        return sulStatus;
    }


}



