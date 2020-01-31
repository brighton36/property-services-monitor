#include "monitor_job.h"

#include "yaml-cpp/yaml.h"

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
  SmtpNotifier notifier;

  // Verify the config :
  try {
    job = MonitorJob(string(argv[1]));
    notifier = SmtpNotifier(job.smtp_params);
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

  // TODO: What to do about these outputs...
  // let's parse them from the json
  fmt::print("To  : {} \nFrom: {}\n", job.to, job.from);
  for (const auto host: job.hosts) {
    fmt::print("  * Host: {} ({})\n", host->label, host->address);
    for(const auto service: host->services) {
      bool is_available = service->IsAvailable();
      // TODO: Why does this not match the email...
      fmt::print("    * {} : {}\n", service->type, is_available ? "OK" : "FAIL");
    }
  }

  // Build Output:
  auto tmpl_data = job.ToJson();

  // Send the email:
  try {
    // TODO :Get it working
    notifier.SendResults(tmpl_data);
  } catch (Poco::Net::SMTPException &e) {
    // TODO: Clean this up with fmt anda macro:
    cout << e.code() << endl;
    cout << e.message() << endl;
    cout << e.what() << endl;
    cout << e.displayText().c_str() << endl;
  }
  catch (Poco::Net::NetException &e) {
    // TODO: Clean this up with fmt anda macro:
    cout << e.code() << endl;
    cout << e.message() << endl;
    cout << e.what() << endl;
    cout << e.displayText().c_str() << endl;
  }
    
  return 0;
}
