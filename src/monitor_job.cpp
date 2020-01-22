#include "monitor_job.h"
#include "yaml-cpp/yaml.h"

using namespace std;

#define MISSING_FIELD "Missing \"{}\" field."
#define MISSING_HOST_FIELD "Missing {} for host \"{}\"."

MonitorJob::MonitorJob(string path) {
	config_path = path;

	const auto config = YAML::LoadFile(config_path);

	if (!config["to"]) 
    throw invalid_argument(fmt::format(MISSING_FIELD, "to"));
	if (!config["from"]) 
    throw invalid_argument(fmt::format(MISSING_FIELD, "from"));
	if (!config["subject"]) 
    throw invalid_argument(fmt::format(MISSING_FIELD, "subject"));
	if (!config["smtp_host"]) 
    throw invalid_argument(fmt::format(MISSING_FIELD, "smtp_host"));
	if (!config["hosts"]) 
    throw invalid_argument(fmt::format(MISSING_FIELD, "hosts"));

	to = config["to"].as<string>();
	from = config["from"].as<string>();
	smtp_host = config["smtp_host"].as<string>();
	subject = config["subject"].as<string>();

	if (config["hosts"].size() < 1) throw invalid_argument("No Hosts to monitor."); 
	
	for (const auto config_host: config["hosts"]) {
		if (!config_host["label"]) 
			throw invalid_argument("One or more hosts are missing a label.");

    const auto label = config_host["label"].as<string>();

		if (!config_host["address"]) 
			throw invalid_argument(fmt::format(MISSING_HOST_FIELD, "address", label));

		if (config_host["services"].size() < 1 || !config_host["services"].IsSequence()) 
			throw invalid_argument(fmt::format(MISSING_HOST_FIELD, "services", label));

    auto address = config_host["address"].as<string>();

		vector<shared_ptr<MonitorServiceBase>> services;

		for (const auto config_service: config_host["services"]) {
      string type;
      
      PTR_UMAP_STR params = make_shared<unordered_map<string, string>>();

      shared_ptr<MonitorServiceBase> service;

      if (config_service.IsMap()) {
        if (!config_service["type"]) 
          throw invalid_argument(
            fmt::format("Missing a service type under host \"{}\"", label)); 

				for(auto it=config_service.begin();it!=config_service.end();++it) {
          const auto param = it->first.as<std::string>();
          const auto value = it->second.as<std::string>();

          if (param == "type")
            type = value;
          else
            params->insert(make_pair(param, value));
				}

        
      } else {
        type = config_service.as<string>();
      }
      
      // TODO: Test to ensure the string exists

      service = MonitorServiceFactory::createInstance(type, address, params);
      
      if (service == nullptr)
        throw invalid_argument(
          fmt::format("Invalid Service encountered for host \"{}\"", label)); 

			services.push_back(service);
    }

		hosts.push_back(make_shared<MonitorHost>( label, address, services ));
	}
}	

// Maybe we can do an eachHost() thing, passing the host and services to the iterator
// that would be useful in the destructor too
void MonitorJob::printtest() { 
  fmt::print("To: {} From: {}\n", to, from);

	for (const auto host: hosts) {
    fmt::print("  * host: {} - {}\n", host->label, host->address);
    fmt::print("  * Services:\n");

		for(const auto service: host->services) {
      fmt::print("    * {}\n", service->type);
      service->IsAvailable();
		}
	}
} 

MonitorHost::MonitorHost(string label, string address, vector<shared_ptr<MonitorServiceBase>> services) {
	this->label = label;
	this->address = address;
	this->services = services;
}

