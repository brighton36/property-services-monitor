#include "monitor_job.h"
#include "yaml-cpp/yaml.h"

using namespace std;

MonitorJob::MonitorJob(string path) {
	config_path = path;

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
		if (config_host["services"].size() < 1 || !config_host["services"].IsSequence()) 
			throw invalid_argument("Missing services for host"); 

    string label = config_host["label"].as<string>();
    string address = config_host["address"].as<string>();

		vector<shared_ptr<MonitorServiceBase>> services;

		for (YAML::detail::iterator_value config_service: config_host["services"]) {
      string type;
      // TODO: Does this need to be pointer... I think it does.
			unordered_map<std::string, std::string> params;
      shared_ptr<MonitorServiceBase> service;

      if (config_service.IsMap()) {
        if (!config_service["type"]) 
          throw invalid_argument("Missing service type");


				for(YAML::const_iterator it=config_service.begin();it!=config_service.end();++it) {
          string param = it->first.as<std::string>();
          string value = it->second.as<std::string>();

          if (param == "type")
            type = value;
          else
            params[param] = value;
				}

        
      } else {
        type = config_service.as<string>();
      }
      
      // TODO: Test to ensure the string exists

      service = MonitorServiceFactory::createInstance(type, address, params);
      
      if (service == nullptr)
        throw invalid_argument("Invalid Host Service encountered"); 

			services.push_back(service);
    }

		hosts.push_back(make_shared<MonitorHost>( label, address, services ));
	}
}	

MonitorJob::~MonitorJob() {
  cout << "~MonitorJob()" << endl;
}

// Maybe we can do an eachHost() thing, passing the host and services to the iterator
// that would be useful in the destructor too
void MonitorJob::printtest() { 
	cout << "To  :" << to << endl << "From:" << from << endl;

	for (shared_ptr<MonitorHost> host: hosts) {
		cout << "  * host: " << host->label << " - " << host->address << endl;
		cout << "  * Services:" << endl;

		for(shared_ptr<MonitorServiceBase> service: host->services) {
			cout << "    * " << service->type << endl;
      service->IsAvailable();
		}
	}
} 

MonitorHost::MonitorHost(string label, string address, vector<shared_ptr<MonitorServiceBase>> services) {
	this->label = label;
	this->address = address;
	this->services = services;
}

MonitorHost::~MonitorHost() {
  cout << "~MonitorHost()" << endl;
}

