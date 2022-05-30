// Copyright 2021-2022 Sophos Limited. All rights reserved.


#define TABLE_PLUGIN_NAME(name)                                                                                        \
private:                                                                                                               \
    std::string m_name = { #name };                                                                                    \
public:                                                                                                                \
    std::string& GetName() override { return m_name; }
#define TABLE_PLUGIN_COLUMNS(...)                                                                                      \
private:                                                                                                               \
    std::vector<OsquerySDK::TableColumn> m_columns = { __VA_ARGS__ };                                                  \
public:                                                                                                                \
    std::vector<OsquerySDK::TableColumn>& GetColumns() override { return m_columns; }
#define TABLE_PLUGIN_COLUMN(name, type, option)                                                                        \
    OsquerySDK::TableColumn { #name, type, option }
#define TABLE_PLUGIN_COLUMN_SNAKE_CASE(name, sc_name, type, option)                                                                        \
    OsquerySDK::TableColumn { #name, type, option }, OsquerySDK::TableColumn { #sc_name, type, option }
