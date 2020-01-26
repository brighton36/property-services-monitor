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

  // Initialize objects, verify the config :
  MonitorJob job;
  SmtpNotifier notifier;

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

  // Now run our tests:
  fmt::print("To  : {} \nFrom: {}\n", job.to, job.from);

  for (const auto host: job.hosts) {
    fmt::print("  * Host: {} ({})\n", host->label, host->address);

    for(const auto service: host->services) {
      const auto is_up = service->IsAvailable(); 

      fmt::print("    * {} : {}\n", service->type, is_up ? "OK" : "FAIL");
    }
  }

  // Send the email:
  try {
    string content = "Test Message";

    Poco::Net::MailMessage message;
    message.setSender(job.from);
    message.addRecipient(
      Poco::Net::MailRecipient(Poco::Net::MailRecipient::PRIMARY_RECIPIENT, job.to));
    message.setSubject(job.subject);
    message.setContentType("text/plain; charset=UTF-8");
    message.setContent(content, Poco::Net::MailMessage::ENCODING_8BIT);

    notifier.Send(&message);
  } catch (Poco::Net::SMTPException &e) {
    cout << e.code() << endl;
    cout << e.message() << endl;
    cout << e.what() << endl;
    cout << e.displayText().c_str() << endl;
  }
  catch (Poco::Net::NetException &e) {
    cout << e.code() << endl;
    cout << e.message() << endl;
    cout << e.what() << endl;
    cout << e.displayText().c_str() << endl;
  }
    
  return 0;
}
