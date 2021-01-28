/******************************************************************************************************

Copyright 2018-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "UpdateScheduler/IState.h"

// Tests for classes derived from SEUError are in CompletionReporterTests

#define IDS_RMS_INST_SUCCESS            101
#define IDS_RMS_INST_ERROR              103
#define IDS_RMS_UNINST_ERROR            104
#define IDS_RMS_INST_GENERAL_ERROR      106
#define IDS_RMS_DWNLD_ERROR             107
#define IDS_RMS_DWNLD_CANCELLED         108
#define IDS_RMS_REBOOTREQUIRED          109
#define IDS_RMS_NOVALIDLOCATIONS        110
#define IDS_RMS_NOLOCATIONS_FOR_PACKAGE 111
#define IDS_RMS_CONNECTIONFAILED        112
#define IDS_RMS_NOLOCATIONS_AT_ALL      113
#define IDS_RMS_MISSING_PRODUCT         114


#define ExitCode_Success                0
#define ExitCode_NoPolicy               1
#define ExitCode_SyncFailure            2
#define ExitCode_UninstallFailure       3
#define ExitCode_InstallFailure         4
#define ExitCode_RebootRequired         5
#define ExitCode_MissingProduct         6

class SEUInstallError : public SEUError
{
public:
	SEUInstallError(const std::string& product, const std::string& installResult = "") : SEUError(ExitCode_InstallFailure, IDS_RMS_INST_ERROR, "Install.Failure")
	{
		// "ERROR:   Product installation failed: %1"
		inserts_.push_back(product);

		if (installResult.length() > 0)
		{
			inserts_.push_back(installResult);
		}
	}
};

class SEUUninstallError : public SEUError
{
public:
	SEUUninstallError(const std::string& product, const std::string& uninstallResult = "") : SEUError(ExitCode_UninstallFailure, IDS_RMS_UNINST_ERROR, "Uninstall.Failure")
	{
		// "ERROR:   Product uninstallation failed: %1"
		inserts_.push_back(product);

		if (uninstallResult.length() > 0)
		{
			inserts_.push_back(uninstallResult);
		}
	}
};

class SEUNoNetworkError : public SEUError
{
public:
    SEUNoNetworkError() : SEUError(ExitCode_SyncFailure, IDS_RMS_CONNECTIONFAILED, "NoNetwork")
    {
    }
};

class SEUSyncError : public SEUError
{
public:
	SEUSyncError(const std::string& product, const std::string& url) : SEUError(ExitCode_SyncFailure, IDS_RMS_DWNLD_ERROR, "SDDSDownloadFailed")
	{
		// ERROR:   Download of %1 failed from server %2
		inserts_.push_back(product);
		inserts_.push_back(url);
	}
};

class SEURebootRequired : public SEUError
{
public:
	SEURebootRequired() : SEUError(ExitCode_RebootRequired, IDS_RMS_REBOOTREQUIRED, "AutoUpdate.RebootNeeded")
	{
		// WARNING: Restart needed for updates to take effect
	}
};

class SEUNoPolicyError : public SEUError
{
public:
	SEUNoPolicyError() : SEUError(ExitCode_NoPolicy, IDS_RMS_NOLOCATIONS_AT_ALL, "NoLocations")
	{
		// ERROR:   Could not find a source for updated packages
	}
};

class SEUMissingProductError : public SEUError
{
public:
    SEUMissingProductError(const std::string& rigidName) : SEUError(ExitCode_MissingProduct, IDS_RMS_MISSING_PRODUCT, "ProductMissing")
    {
        // ERROR:   A product was missing when trying to synchronise
        inserts_.push_back(rigidName);
    }
};
