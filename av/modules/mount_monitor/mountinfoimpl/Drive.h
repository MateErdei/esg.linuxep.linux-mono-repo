// Copyright 2022, Sophos Limited.  All rights reserved.

#include "mount_monitor/mountinfo/IMountInfo.h"
#include "DeviceUtil.h"

namespace mount_monitor::mountinfoimpl
{
    class Drive : virtual public mountinfo::IMountPoint
    {
    public:
        /**
             *
             * @param device
             * @param mountPoint
             * @param type
         */
        Drive(std::string device, std::string mountPoint, std::string type, bool isDirectory);

        /**
             * Constructs Drive of mount point nearest to childPath.
             * Throws if unable to access /proc/mounts or no parent found (should always find "/").
             *
             * @param childPath
         */
        explicit Drive(const std::string& childPath);

        ~Drive() override = default;

        [[nodiscard]] std::string mountPoint() const override;

        [[nodiscard]] std::string device() const override;

        [[nodiscard]] std::string filesystemType() const override;

        [[nodiscard]] bool isHardDisc() const override;

        [[nodiscard]] bool isNetwork() const override;

        [[nodiscard]] bool isOptical() const override;

        [[nodiscard]] bool isRemovable() const override;

        /**
             * @return true if this is a special filesystem mount that we should avoid
             * scanning.
         */
        [[nodiscard]] bool isSpecial() const override;

        [[nodiscard]] bool isDirectory() const override;

        [[nodiscard]] bool isReadOnly() const override;

    private:
        mountinfoimpl::DeviceUtilSharedPtr m_deviceUtil;
        std::string m_mountPoint;
        std::string m_device;
        std::string m_fileSystem;
        bool m_isDirectory;
        bool m_isReadOnly;
    };
}