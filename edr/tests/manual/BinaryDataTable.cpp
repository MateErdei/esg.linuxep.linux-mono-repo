#include <sstream>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#include <osquery/tables.h>
#include <osquery/sql/dynamic_table_row.h>
#include <osquery/flagalias.h>
#include <osquery/flags.h>
#include <extensions/interface.h>
#include <osquery/flags.h>
#include <osquery/sdk/sdk.h>
#include <osquery/system.h>
#pragma GCC diagnostic pop
#include <iostream>
#include <fstream>
namespace osquery
{
    FLAG(bool, decorations_top_level, false, "test");
}


// Note 2: Define at least one plugin or table.
namespace osquery{
    class BinaryDataTablePlugin : public osquery::TablePlugin {
    private:
        TableColumns columns() const override {
            return {

                    std::make_tuple("data", TEXT_TYPE, ColumnOptions::REQUIRED),
                    std::make_tuple("count", INTEGER_TYPE, ColumnOptions::REQUIRED)
            };
        }

        TableRows generate(QueryContext& request) override {
            TableRows results;
            std::set<std::string> paths = request.constraints["count"].getAll(EQUALS);
            if (paths.empty())
            {
                return results;
            }

            std::stringstream count{*paths.begin()};
            int countv{0};
            count >> countv;
            auto r = osquery::make_table_row();
            std::ifstream input( "/usr/bin/touch", std::ios::binary );

            // copies all data into buffer
            std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(input), {});
            std::string data;
            int iterator =0;
            for (auto character : buffer)
            {
                if (iterator>countv)
                {
                    break;
                }

                data += character;
                iterator += 1;
            }
            r["data"] = data;
            r["count"] = countv;
            results.push_back(std::move(r));
            return results;
        }
    };
    REGISTER_EXTERNAL(BinaryDataTablePlugin, "table", "binarydatatable");
}

// Note 3: Use REGISTER_EXTERNAL to define your plugin or table.
using namespace osquery;
int main(int argc, char* argv[]) {
  osquery::Initializer runner(argc, argv, ToolType::EXTENSION);

  auto status = startExtension("binarydatatable", "0.0.1");
  if (!status.ok()) {
    LOG(ERROR) << status.getMessage();
    runner.requestShutdown(status.getCode());
  }

  // Finally wait for a signal / interrupt to shutdown.
  runner.waitThenShutdown();
  return 0;
}
