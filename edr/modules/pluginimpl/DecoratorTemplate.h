/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/
namespace Plugin {
    namespace DecoratorTemplate {

        const std::string decorators{"{"
                                         "\"interval\": {"
                                             "\"3600\": ["
                                                 "\"SELECT endpoint_id AS eid from sophos_endpoint_info\","
                                                 "\"SELECT\\n    interface_details.mac AS mac_address,\\n    interface_addresses.mask AS ip_mask,\\n    interface_addresses.address AS ip_address\\nFROM\\n    interface_addresses\\n    JOIN interface_details ON interface_addresses.interface = interface_details.interface\\nWHERE\\n    ip_address NOT LIKE '127.%'\\n    AND ip_address NOT LIKE '%:%'\\n    AND ip_address NOT LIKE '169.254.%'\\n    AND ip_address NOT LIKE '%.1'\\nORDER BY\\n    interface_details.last_change\\nLIMIT\\n    1\","
                                                 "\"SELECT\\n    user AS username\\nFROM\\n    logged_in_users\\nWHERE\\n    (\\n        type = 'user'\\n        OR type = 'active'\\n    )\\nORDER BY\\n    time DESC\\nLIMIT\\n    1\""
                                             "]"
                                         "},"
                                         "\"load\": ["
                                             "\"SELECT (unix_time - (select total_seconds from uptime)) AS boot_time FROM time\","
                                             "\"SELECT\\n    CASE\\n        WHEN computer_name == '' THEN hostname\\n        ELSE computer_name\\n    END AS hostname\\nFROM\\n    system_info\","
                                             "\"SELECT\\n    name AS os_name,\\n    version AS os_version,\\n    platform AS os_platform\\nFROM\\n    os_version\\nLIMIT\\n    1\","
                                             "\"SELECT\\n    CASE \\n        WHEN upper(platform) == 'WINDOWS' AND upper(name) LIKE '%SERVER%' THEN 'server' \\n        WHEN upper(platform) == 'WINDOWS' AND upper(name) NOT LIKE '%SERVER%' THEN 'client' \\n        WHEN upper(platform) == 'DARWIN' THEN 'client' \\n        WHEN (SELECT count(*) FROM system_info WHERE cpu_brand LIKE '%Xeon%') == 1 THEN 'server' \\n        WHEN (SELECT count(*) FROM system_info WHERE hardware_vendor LIKE '%VMWare%') == 1 THEN 'server' \\n        WHEN (SELECT count(*) FROM system_info WHERE hardware_vendor LIKE '%QEMU%') == 1 THEN 'server' \\n        WHEN (\\n            (SELECT obytes FROM interface_details ORDER by obytes DESC LIMIT 1) > (SELECT ibytes FROM interface_details ORDER by ibytes DESC LIMIT 1)\\n            ) == 1 THEN 'server'\\n        ELSE 'client'\\n    END AS 'os_type'\\nFROM 'os_version';\","
                                             "\"SELECT endpoint_id AS eid from sophos_endpoint_info\","
                                             "\"SELECT\\n    interface_details.mac AS mac_address,\\n    interface_addresses.mask AS ip_mask,\\n    interface_addresses.address AS ip_address\\nFROM\\n    interface_addresses\\n    JOIN interface_details ON interface_addresses.interface = interface_details.interface\\nWHERE\\n    ip_address NOT LIKE '127.%'\\n    AND ip_address NOT LIKE '%:%'\\n    AND ip_address NOT LIKE '169.254.%'\\n    AND ip_address NOT LIKE '%.1'\\nORDER BY\\n    interface_details.last_change\\nLIMIT\\n    1\","
                                             "\"SELECT\\n    user AS username\\nFROM\\n    logged_in_users\\nWHERE\\n    (\\n        type = 'user'\\n        OR type = 'active'\\n    )\\nORDER BY\\n    time DESC\\nLIMIT\\n    1\","
                                             "\"SELECT '1.1.19' query_pack_version\""
                                         "]"
                                     "}"};

    } // namespace DecoratorTemplate
} // namespace Plugin