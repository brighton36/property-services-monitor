#include "property-services-monitor.h"
#include "notifier_smtp.h"

#include "Poco/Net/NetException.h"
#include "Poco/Net/MailRecipient.h"

using namespace std;

shared_ptr<MonitorServiceFactory::map_type> MonitorServiceFactory::map = nullptr;

int main(int argc, char* argv[]) {
  vector<string> args(argv + 1, argv + argc);

  // Help wanted?  
  if ( (args.size() == 0 ) || has_any(args, {"-h", "--help"}) ) {
    string help = fmt::format("Usage: {} [-o] [FILE]\n"
      "A lightweight service availability checking tool.\n\n"
      "-o           Output a terse summary of the notification contents to STDOUT.\n"
      "--help       Display this help and exit.\n\n"
      "The supplied FILE is expected to be a yaml-formatted service monitor definition file.\n"
      "(See https://en.wikipedia.org/wiki/YAML for details on the YAML file format.)\n\n"
      "At the root of the config file, two parameters are required: \"notification\""
      " and \"hosts\".\n\n", argv[0]);

    help.append(NotifierSmtp::Help());

    help.append(
      "\nThe \"hosts\" (sequence) is expected to provide an itemization of the systems being monitored.\n"
      "Each host item in the sequence is itself a (map). \n\n"
      "The format of each host's (map) is as follows:\n"
      " * address     (required) The FQDN or IP address of the host.\n"
      " * label       (required) The human-readable moniker of this host. Used in the output report.\n"
      " * description (required) A description of the host, for use in the output report.\n"
      " * services    (required) A (sequence) of services to test, that are expected to be running on\n"
      "                          this host. See below for details.\n\n"
    );

    auto services = MonitorServiceFactory::getRegistrations();
    string services_joined = fmt::format("\"{}\"", services[0]);
    for( unsigned int i = 1; i < services.size(); i++)
      services_joined.append(fmt::format(", \"{}\"", services[i]));

    help.append(fmt::format(
      "The \"services\" (sequence) is expected to provide an itemization of the service being tested\n"
      "for availability. Each service in the sequence is itself a (map).\n\n"
      "The format of service's (map) is as follows:\n"
      " * type        (required) The type of service being tested. The type must be one of the\n"
      "                          following supported types: {}.\n\n"
      "                          Depending on the value of this parameter, additional sequence\n"
      "                          options may be available. in this service's map section. What\n"
      "                          follows are type-specific parameters.\n", 
      services_joined ));

    for (string service : services)
      help.append(fmt::format( "\nFor \"{}\" service types:\n{}", 
        service,MonitorServiceFactory::getHelp(service) ));

    help.append("\n"
      "The DATETIME format:\n"
      "  This string format supports multiple descriptions of relative dates and times, for use\n"
      "  in your configurations. Some examples include : \"Yesterday at 11:00a\", \"2 hours ago\"\n"
      "  \"11:00 P, 2 days prior\", \"Today at 1:05 P\", and even\n"
      "  \"2 days, 1 hour, 10 minutes back...\". All times are relative to \"now\", the time at.\n"
      "  which the program is being run.\n"
      "\n"
      "The DURATION format:\n"
      "  This string indicates a number of seconds, for use in your configurations. Some \n"
      "  examples include: \"30 seconds\", \"2 hours\", \"20 days\", \"1 month\", and even \n"
      "  \"3 weeks, 2 hours, 30 minutes, 10 seconds\".\n");

    help.append("\n"
      "For more information about this program, see the github repo at:\n"
      "  https://github.com/brighton36/property-services-monitor/\n" );

    cout << help; 

    return 1;
  }

  MonitorJob job;
  NotifierSmtp notifier;
  string config_path;
  string base_path;

  try {
    // Figure out what config file we were provided. We take the first parameter
    // that doesn't start with a dash:
    for(string a : args) if (a.at(0) != '-') { config_path = a; break; }

    if (config_path.empty())
      throw invalid_argument("Missing a configuration file. See help for details.");

    if (!pathIsReadable(config_path))
      throw invalid_argument(
         "Unable to read the supplied configuration file. See help for details.");

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
  auto tmpl_data = job.toJson();

  if (has_any(args, {"-o"})) {
    // This is mostly for debugging I suppose. It's a bit underwhelming.
    // Perhaps more should be done here.
    fmt::print("To  : {} \nFrom: {}\n", notifier.to, notifier.from);

    for (auto& host : tmpl_data["hosts"].items()) {
      auto host_values = host.value();
      fmt::print("  * Host: {} ({})\n", (string) host_values["label"], (string) host_values["address"]);
      for (auto& service : host_values["services"].items()) {
        auto service_values = service.value();
        fmt::print("    * {} : {}\n", (string) service_values["type"], 
          (service_values["is_up"]) ? "OK" : "FAIL");
      }
    }
  }

  // Send the email:
  try {
    notifier.sendResults(&tmpl_data);
  } catch (const Poco::Exception &e) {
    fmt::print(cerr, "POCO Exception Encountered: {} \n", e.displayText() );
    return 1;
  }
  catch (const exception &e) {
    fmt::print(cerr, "General Exception Encountered: {} \n", e.what() );
    return 1;
  }
    
  return 0;
}
