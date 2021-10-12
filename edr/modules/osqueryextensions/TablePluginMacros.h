// Copyright 2021-2021 Sophos Limited. All rights reserved.


#define TABLE_PLUGIN_NAME(name)                                                                                        \
    std::string GetName() { return #name; }
#define TABLE_PLUGIN_COLUMNS(...)                                                                                      \
    std::vector<OsquerySDK::TableColumn> GetColumns() { return std::vector<OsquerySDK::TableColumn>{ __VA_ARGS__ }; }
#define TABLE_PLUGIN_COLUMN(name, type, option)                                                                        \
    OsquerySDK::TableColumn { #name, type, option }
#define TABLE_PLUGIN_COLUMN_SNAKE_CASE(name, sc_name, type, option)                                                                        \
    OsquerySDK::TableColumn { #name, type, option }, OsquerySDK::TableColumn { #sc_name, type, option }
