//------------------------------------------------------------------------------
// Copyright 2012-2021 Sophos Limited. All rights reserved.
//
// Sophos is a registered trademark of Sophos Limited and Sophos Group. All
// other product and company names mentioned are trademarks or registered
// trademarks of their respective owners.
//------------------------------------------------------------------------------

#pragma once

#include <functional>
#include <string>
#include <vector>

class RegistryHandleView
{
public:
    explicit RegistryHandleView(HKEY key);

    struct Value {
        std::string name;
        unsigned int type;
        // TODO: use std::variant when we switch to C++17
        uint32_t dword;
        uint64_t qword;
        std::vector<unsigned char> bin;
        std::string str;
        std::vector<std::string> multi;
    };
    virtual size_t VisitValues(std::function<void(const Value&)> visitor);

    virtual int ReadIntValue(const wchar_t* valueName);
    virtual int ReadIntValue(const wchar_t* valueName, int defaultValue);

    virtual DWORD ReadDwordValue(const wchar_t* valueName);
    virtual DWORD ReadDwordValue(const wchar_t* valueName, DWORD defaultValue);

    virtual std::uint64_t ReadQwordValue(const wchar_t* valueName);
    virtual std::uint64_t ReadQwordValue(const wchar_t* valueName, std::uint64_t defaultValue);

    virtual std::string ReadStringValue(const wchar_t* valueName);
    virtual std::string ReadStringValue(const wchar_t* valueName, const std::string& defaultValue);
    virtual std::vector<std::string> ReadMultiStringValue(const wchar_t* valueName);
    virtual int CountSubKeys();
    virtual std::vector<std::string> ReadSubKeyNames();
    virtual std::vector<std::string> ReadValueNames();

    virtual void WriteStringValue(const wchar_t* valueName, const std::string& input);
    virtual void WriteMultiStringValue(const wchar_t* valueName, std::vector<std::string> const& input);
    virtual void WriteDwordValue(const wchar_t* valueName, DWORD input);
    virtual void WriteQwordValue(const wchar_t* valueName, std::uint64_t input);

    // Delete a child key; does not throw if the child key already does not exist
    virtual void DeleteSubKey(const wchar_t* keyName);
    // Delete a child key recursively; does not throw if the child key already does not exist
    virtual void DeleteTree(const wchar_t* keyName);
    // Delete a value; returns true on success, returns false if the value already does not exist, throws on any other failure
    virtual bool DeleteValue(const wchar_t* valueName);

    HKEY m_key;

private:
    DWORD ReadStringValueByteLength(const wchar_t* valueName);
    std::string ReadStringValueData(const wchar_t* valueName, DWORD length);
};

class RegistryHandle :
    public RegistryHandleView
{
public:
    explicit RegistryHandle(HKEY key);
    RegistryHandle(const RegistryHandle&) = delete;
    RegistryHandle& operator=(const RegistryHandle&) = delete;
    RegistryHandle(RegistryHandle&& other) noexcept;
    RegistryHandle& operator=(RegistryHandle&&) = delete;
    virtual ~RegistryHandle();
};

class RegistryAccess : public RegistryHandle
{
public:
	explicit RegistryAccess(HKEY parentKey, const wchar_t * keyName, REGSAM samDesired, DWORD options = 0);
    explicit RegistryAccess(const RegistryHandle& parentKey, const wchar_t * keyName, REGSAM samDesired, DWORD options = 0);
};

class RegistryReader : public RegistryAccess
{
public:
	explicit RegistryReader(HKEY parentKey, const wchar_t * keyName);
    explicit RegistryReader(const RegistryHandle& parentKey, const wchar_t * keyName);
};

class RegistryWriter : public RegistryAccess
{
public:
	explicit RegistryWriter(HKEY parentKey, const wchar_t * keyName);
    explicit RegistryWriter(const RegistryHandle& parentKey, const wchar_t * keyName);
};

class RegistryCreator : public RegistryHandle
{
public:
	explicit RegistryCreator(HKEY parentKey, const wchar_t * keyName, REGSAM samDesired, DWORD options = 0);
    explicit RegistryCreator(const RegistryHandle& parentKey, const wchar_t * keyName, REGSAM samDesired, DWORD options = 0);
};

bool RegistrySubKeyExists(HKEY hKey, LPCWSTR lpSubKey, REGSAM samDesired);
