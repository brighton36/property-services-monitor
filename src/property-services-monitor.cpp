#include "monitor_job.h"

#include "Poco/Net/NetException.h"
#include "Poco/Net/MailRecipient.h"

using namespace std;

MonitorServiceFactory::map_type * MonitorServiceFactory::map = nullptr;

int main(int argc, char* argv[]) {
  if (argc < 2) {
    fmt::print(cerr, "Usage: {} config.yml\n", argv[0]);
    return 1;
  }

  MonitorJob job;
  NotifierSmtp notifier;
  string base_path;

  // Load and Verify the config :
  try {
    const auto config_path = string(argv[1]);
    const auto config = YAML::LoadFile(config_path);

    base_path = filesystem::path(filesystem::canonical(config_path)).parent_path();


    if (!config["hosts"]) 
      throw invalid_argument(fmt::format(MISSING_FIELD, "hosts"));

    if (!config["notification"]) 
      throw invalid_argument(fmt::format(MISSING_FIELD, "notification"));

    job = MonitorJob(config["hosts"]);
    notifier = NotifierSmtp(base_path, config["notification"]);
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

  // Build Output:
  auto tmpl_data = job.ToJson();

  // TODO: We may want to just dump the text template from the job()
  fmt::print("To  : {} \nFrom: {}\n", notifier.to, notifier.from);

  for (auto& host : tmpl_data["hosts"].items()) {
    auto host_values = host.value();
    fmt::print("  * Host: {} ({})\n", host_values["label"], host_values["address"]);
    for (auto& service : host_values["services"].items()) {
      auto service_values = service.value();
      fmt::print("    * {} : {}\n", service_values["type"], 
        (service_values["is_up"]) ? "OK" : "FAIL");
    }
  }

  // Send the email:
  try {
    notifier.SendResults(&tmpl_data);
  } catch (Poco::Net::SMTPException &e) {
    fmt::print(cerr, "SMTP Exception Encountered: {} {} {} {}\n", 
      e.code(), e.what(), e.message(), e.displayText().c_str() );
    return 1;
  }
  catch (Poco::Net::NetException &e) {
    fmt::print(cerr, "Net Exception Encountered: {} {} {} {}\n", 
      e.code(), e.what(), e.message(), e.displayText().c_str() );
    return 1;
  }
    
  return 0;
}
