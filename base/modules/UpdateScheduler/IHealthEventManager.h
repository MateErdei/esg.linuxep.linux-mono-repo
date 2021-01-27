/* Copyright 2016 Sophos Limited. All rights reserved.
* Sophos and Sophos Anti-Virus are registered trademarks of Sophos Limited.
* All other product and company names mentioned are trademarks or registered
* trademarks of their respective owners.
*/

#pragma once

#include <string>

class IHealthEventManager
{
public:
    IHealthEventManager() {}
    virtual ~IHealthEventManager() {}

    virtual void WriteEvents() = 0;
	virtual void InstallSucceeded(const std::string& rigidName, const std::string& longName) = 0;
    virtual void InstallFailed(const std::string& rigidName, const std::string& longName) = 0;
    virtual void Uninstalled(const std::string& rigidName, const std::string& longName) = 0;
    virtual void OptionalRebootRequired() = 0;
    virtual void MandatoryRebootRequired() = 0;
    virtual void CrtSucceeded() = 0;
    virtual void CrtFailed() = 0;
};