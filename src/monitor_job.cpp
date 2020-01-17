#include <iostream>

#include "yaml-cpp/yaml.h"
#include "monitor_job.h"

using namespace std;

MonitorJob::MonitorJob(char *szPath) {
	config_path = string(szPath);

	YAML::Node config = YAML::LoadFile(config_path);

	if (!config["to"]) 
		throw invalid_argument("Missing \"to\" field.");
	if (!config["from"]) 
		throw invalid_argument("Missing \"from\" field.");
	if (!config["subject"]) 
		throw invalid_argument("Missing \"subject\" field.");
	if (!config["smtp_host"]) 
		throw invalid_argument("Missing \"smtp_host\" field.");
	if (!config["hosts"]) 
		throw invalid_argument("Missing \"hosts\" field.");

	to = config["to"].as<string>();
	from = config["from"].as<string>();
	smtp_host = config["smtp_host"].as<string>();
	subject = config["subject"].as<string>();

	if (config["hosts"].size() < 1) throw invalid_argument("No Hosts to monitor"); 
	
	for (YAML::detail::iterator_value config_host: config["hosts"]) {
		if (!config_host["label"]) 
			throw invalid_argument("Missing label for host.");
		if (!config_host["address"]) 
			throw invalid_argument("Missing address for host.");
		if (config_host["services"].size() < 1) 
			throw invalid_argument("Missing services for host"); 

    string label = config_host["label"].as<string>();
    string address = config_host["address"].as<string>();

		vector<MonitorServiceBase *> services;
		for (YAML::detail::iterator_value config_service: config_host["services"]) {
      string service_type = config_service.as<string>();
      MonitorServiceBase *service;

      if (service_type == string("ping"))
        service = new MonitorServicePing(address);
      else if (service_type == string("web"))
        service = new MonitorServiceWeb(address);
      else
        throw invalid_argument("Invalid service encountered."); 

			services.push_back(service);
    }

		hosts.push_back(new MonitorHost( label, address, services ));
	}
}	

MonitorJob::~MonitorJob() {
	for (MonitorHost *host: hosts)
    delete host;
}

// Maybe we can do an eachHost() thing, passing the host and services to the iterator
// that would be useful in the destructor too
void MonitorJob::printtest() { 
	cout << "To  :" << to << endl << "From:" << from << endl;

	for (MonitorHost *host: hosts) {
		cout << "  * host: " << host->label << " - " << host->address << endl;
		cout << "  * Services:" << endl;

		for(MonitorServiceBase *service: host->services) {
			cout << "    * " << service->type << endl;
      if (service->type == string("ping")) {
        MonitorServicePing * service_ping = (MonitorServicePing *)service;
        service_ping->IsAvailable();
      }
		}
	}
} 

MonitorHost::MonitorHost(string label, string address, vector<MonitorServiceBase *> services) {
	this->label = label;
	this->address = address;
	this->services = services;
}

MonitorHost::~MonitorHost() {
  for(MonitorServiceBase *service: services)
    delete service;
}

