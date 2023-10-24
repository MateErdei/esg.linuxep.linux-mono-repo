// Copyright 2021-2023 Sophos Limited. All rights reserved.

#pragma once

#include "TablePluginMacros.h"

#ifdef SPL_BAZEL
#include "common/livequery/include/OsquerySDK/OsquerySDK.h"
#else
#include "OsquerySDK/OsquerySDK.h"
#endif

namespace OsquerySDK
{
    class SophosAVDetectionTable : public OsquerySDK::TablePluginInterface
    {
    public:
        SophosAVDetectionTable() = default;

        /*
         * @description
         * Detection events from Sophos journals.
         *
         * @example
         * SELECT
         *     *
         * FROM
         *     sophos_detections_journal
         * WHERE
         *     time > 1559641265
         */
        TABLE_PLUGIN_NAME(sophos_detections_journal)
        TABLE_PLUGIN_COLUMNS(
            /*
             * @description
             * The ID of the detection event
             */
            TABLE_PLUGIN_COLUMN(rowid, UNSIGNED_BIGINT_TYPE, ColumnOptions::HIDDEN),
            /*
             * @description
             * The event time (unix epoch) the journal was created
             * If no time constraint is specified the default constraint will be to retrieve events starting at 'now - 15
             * minutes' The only constraints supported for time are:
             *   - EQUAL
             *   - GREATER THAN
             *   - GREATER THAN OR EQUAL
             *   - LESS THAN
             *   - LESS THAN OR EQUAL
             */
            TABLE_PLUGIN_COLUMN(time, UNSIGNED_BIGINT_TYPE, ColumnOptions::INDEX),
            /*
             * @description
             * The raw JSON string containing all captured data of the event
             */
            TABLE_PLUGIN_COLUMN(raw, TEXT_TYPE, ColumnOptions::DEFAULT),
            /*
             * @description
             * Field to indicate query should use the JRL for the query id
             */
            TABLE_PLUGIN_COLUMN(query_id, TEXT_TYPE, ColumnOptions::DEFAULT),
            /*
             * @description
             * The detection name, aka rule id, threat name, trigger id
             */
            TABLE_PLUGIN_COLUMN(detection_name, TEXT_TYPE, ColumnOptions::DEFAULT),
            /*
             * @description
             * A thumbprint that identifies the particular cause of this detection. For a static PE file detection it
             * is the hash of the file, for AMSI the hash of the outer script-analogue, and for behavioural the hash
             * of the trigger key.
             */
            TABLE_PLUGIN_COLUMN(detection_thumbprint, TEXT_TYPE, ColumnOptions::DEFAULT),
            /*
             * @description
             * The source of the threat detection.
             *
             * One of the following values:
             *  - ML
             *  - SAV
             *  - Rep
             *  - Blocklist
             *  - AppWL
             *  - AMSI
             *  - IPS
             *  - Behavioral
             *  - VDL
             *  - HBT
             *  - MTD
             *  - Web
             *  - Device Control
             *  - HMPA
             */
            TABLE_PLUGIN_COLUMN(threat_source, TEXT_TYPE, ColumnOptions::DEFAULT),
            /*
             * @description
             * The detection threat type.
             *
             * One of the following values:
             *  - Malware
             *  - Pua
             *  - Suspicious Behaviour
             *  - Blocked
             *  - Amsi Protection
             *  - App
             *  - Data Loss Prevention
             *  - Device Control
             */
            TABLE_PLUGIN_COLUMN(threat_type, TEXT_TYPE, ColumnOptions::DEFAULT),
            /*
             * @description
             * Security identifier of the user logged in at the time of the threat detection.
             */
            TABLE_PLUGIN_COLUMN(sid, TEXT_TYPE, ColumnOptions::DEFAULT),
            /*
             * @description
             * Monitor mode is an enum with values (Disabled=0, Enabled=1)
             */
            TABLE_PLUGIN_COLUMN(monitor_mode, INTEGER_TYPE, ColumnOptions::DEFAULT),
            /*
             * @description
             * The JSON string containing the primary item of the event
             */
            TABLE_PLUGIN_COLUMN(primary_item, TEXT_TYPE, ColumnOptions::DEFAULT),
            /*
             * @description
             * The type of the primary item.
             *
             * One of the following values:
             *  - File
             *  - Process
             *  - Url
             *  - Network
             *  - Registry
             *  - Thread
             *  - Device
             */
            TABLE_PLUGIN_COLUMN(primary_item_type, TEXT_TYPE, ColumnOptions::DEFAULT),
            /*
             * @description
             * Identifer of the item
             */
            TABLE_PLUGIN_COLUMN(primary_item_name, TEXT_TYPE, ColumnOptions::DEFAULT),
            /*
             * @description
             * Sophos PID of the accessing process (if known)
             */
            TABLE_PLUGIN_COLUMN(primary_item_spid, TEXT_TYPE, ColumnOptions::DEFAULT),
            /*
             * @description
             * string value either on_access or on_demand  (if known) values set in av repo modules/scan_messages/ScanType.h
             */
                TABLE_PLUGIN_COLUMN(av_scan_type, TEXT_TYPE, ColumnOptions::DEFAULT),
            /*
            * @description
            * PID of the process that triggered an on access detection (if known)
            */
                TABLE_PLUGIN_COLUMN(pid, INTEGER_TYPE, ColumnOptions::DEFAULT),
            /*
            * @description
            * Path of executable that triggered an on access detection (if known)
            */
                TABLE_PLUGIN_COLUMN(process_path, TEXT_TYPE, ColumnOptions::DEFAULT),
            /*
            * @description
            * quarantine_success is an enum with values (false=0, true=1)
            */
                TABLE_PLUGIN_COLUMN(quarantine_success, TEXT_TYPE, ColumnOptions::DEFAULT))
        OsquerySDK::TableRows Generate(OsquerySDK::QueryContextInterface& context) override;
    };
}
