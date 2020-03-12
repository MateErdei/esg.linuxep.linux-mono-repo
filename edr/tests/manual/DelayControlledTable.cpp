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

namespace osquery
{
    FLAG(bool, decorations_top_level, false, "test");
}


// Note 2: Define at least one plugin or table.
namespace osquery{
    class DelayTablePlugin : public osquery::TablePlugin {
    private:
        TableColumns columns() const override {
            return {
                    std::make_tuple("start", BIGINT_TYPE, ColumnOptions::DEFAULT),
                    std::make_tuple("stop", BIGINT_TYPE, ColumnOptions::DEFAULT),
                    std::make_tuple("delay", INTEGER_TYPE, ColumnOptions::REQUIRED)
            };
        }

        TableRows generate(QueryContext& request) override {
            TableRows results;
            std::set<std::string> paths = request.constraints["delay"].getAll(EQUALS);
            if (paths.empty())
            {
                return results;
            }

            std::stringstream delays{*paths.begin()};
            int delayv{0};
            delays >> delayv;

            size_t start = time(NULL);
            std::this_thread::sleep_for(std::chrono::seconds(delayv));
            size_t stop = time(NULL);
            auto r = osquery::make_table_row();
            r["start"] = BIGINT(start);
            r["stop"] = BIGINT(stop);
            r["delay"] = INTEGER(delayv);
            results.push_back(std::move(r));
            return results;
        }
    };
    REGISTER_EXTERNAL(DelayTablePlugin, "table", "delaytable");
}

// Note 3: Use REGISTER_EXTERNAL to define your plugin or table.
using namespace osquery;
int main(int argc, char* argv[]) {
  osquery::Initializer runner(argc, argv, ToolType::EXTENSION);

  auto status = startExtension("delaytable", "0.0.1");
  if (!status.ok()) {
    LOG(ERROR) << status.getMessage();
    runner.requestShutdown(status.getCode());
  }

  // Finally wait for a signal / interrupt to shutdown.
  runner.waitThenShutdown();
  return 0;
}
