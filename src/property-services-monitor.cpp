#include "property-services-monitor.h"

#include "Poco/Net/NetException.h"
#include "Poco/Net/MailRecipient.h"

#define INCLUDES_HELP(v) (find_if((v).begin(), (v).end(), \
  [] (string s) { return ((s == "-h") || (s == "--help")); } ) != (v).end())

using namespace std;

shared_ptr<MonitorServiceFactory::map_type> MonitorServiceFactory::map = nullptr;

bool pathIsReadable(string path) {
	filesystem::path p(path);

	error_code ec;
	auto perms = filesystem::status(p, ec).permissions();

	return ( (ec.value() == 0) && (
    (perms & filesystem::perms::owner_read) != filesystem::perms::none &&
    (perms & filesystem::perms::group_read) != filesystem::perms::none &&
    (perms & filesystem::perms::others_read) != filesystem::perms::none ) );
}

int main(int argc, char* argv[]) {
	vector<string> args(argv + 1, argv + argc);
  
  // Help wanted?  
  if ( (args.size() == 0 ) || INCLUDES_HELP(args) ) {
    string help = fmt::format("Usage: {} [config.yml]\n\n"
      "The supplied argument is expected to be a yaml-formatted service monitor definition file.\n"
      "(See https://en.wikipedia.org/wiki/YAML for details on the YAML file format.)\n\n"
      "The following sections and parameters are supported in your supplied config file.\n\n"
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
	
		string services_joined = accumulate( next(services.begin()), services.end(), 
			services[0], [](string a, string b) { return fmt::format("\"{}\", \"{}\"",a,b);} );

    help.append(fmt::format(
      "The \"services\" (sequence) is expected to provide an itemization of the service being tested\n"
      "for availability. Each service in the sequence is itself a (map).\n\n"
      "The format of service's (map) is as follows:\n"
      " * type           (required) The type of service being tested. The type must be one of the\n"
      "                             following supported types: {}.\n\n"
			"                             Depending on the value of this parameter, additional sequence\n"
			"                             options may be available. in this service's map section. What\n"
			"                             follows are type-specific parameters.\n", 
			services_joined ));

    for (string service : services)
			help.append(fmt::format( "\n For \"{}\" service types:\n{}", 
				service,MonitorServiceFactory::getHelp(service) ));

    help.append(
			"\nFor more information about this program, see the github repo at:\n"
			"  https://github.com/brighton36/property-services-monitor/\n" );

    cout << help; 

    return 1;
  }

  // Seems like we were given a configuration file to parse:
  const auto config_path = args[0];

  MonitorJob job;
  NotifierSmtp notifier;
  string base_path;

  // Load and Verify the config :
  try {
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

  // TODO: We should just dump the text template from the job(). Or, maybe have a text output option
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
    notifier.sendResults(&tmpl_data);
  } catch (Poco::Net::SMTPException &e) {
    fmt::print(cerr, "SMTP Exception Encountered: {} {} {} {}\n", 
      e.code(), e.what(), e.message(), e.displayText().c_str() );
    return 1;
  } catch (Poco::FileNotFoundException &e) {
    // TODO: Catch all these Poco exceptions into this:
    fmt::print(cerr, "Exception Encountered: {} \n", e.displayText() );
    return 1;
  }
  catch (Poco::Net::NetException &e) {
    fmt::print(cerr, "Net Exception Encountered: {} {} {} {}\n", 
      e.code(), e.what(), e.message(), e.displayText().c_str() );
    return 1;
  }
    
  return 0;
}
