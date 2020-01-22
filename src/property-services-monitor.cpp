#include "monitor_job.h"
#include "yaml-cpp/yaml.h"

using namespace std;

MonitorServiceFactory::map_type * MonitorServiceFactory::map = nullptr;

int main(int argc, char* argv[]) {
  if (argc < 2) {
    fmt::print(cerr, "Usage: {} config.yml\n", argv[0]);
    return 1;
  }

  shared_ptr<MonitorJob> job;

  try {
    job = make_shared<MonitorJob>(string(argv[1]));

    fmt::print("To  : {} \nFrom: {}\n", job->to, job->from);

    for (const auto host: job->hosts) {
      fmt::print("  * host: {} - {}\n", host->label, host->address);
      fmt::print("  * Services:\n");

      for(const auto service: host->services) {
        const auto is_up = service->IsAvailable(); 

        fmt::print("    * {} : {}\n", service->type, is_up ? "OK" : "FAIL");
      }
    }

  } catch(const YAML::Exception& e) {
    fmt::print(cerr, "YAML Exception: {}\n", e.what());
    return 1;
  } catch(const invalid_argument& e) {
    fmt::print(cerr, "invalid_argument Encountered: {}\n", e.what());
    return 1;
  } catch(const exception& e) {
    fmt::print(cerr, "General Exception Encountered: {}\n", e.what());
    return 1;
  }

  return 0;
}
