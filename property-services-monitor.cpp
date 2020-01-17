#include <iostream>
#include <unordered_map> 

#include "yaml-cpp/yaml.h"

using namespace std;

class MonitorHost { 
  public:

  string label, address;
  vector<string> services;
  
	MonitorHost(string label, string address, vector<string> services) {
    this->label = label;
    this->address = address;
    this->services = services;
  }
};

class MonitorJob { 
	public: 

	string config_path, to, from, smtp_host, subject;
  vector<MonitorHost> hosts;
  vector<vector<string>*> host_services;

	MonitorJob(char *szPath) {
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

      vector<string> services;
      // TODO: Maybe we want to create service objects here...
      // https://www.boost.org/doc/libs/1_51_0/doc/html/boost_asio/example/icmp/ping.cpp
      // libasio-dev
      // or poco:
      // https://stackoverflow.com/questions/49371450/why-getting-ioexception-when-pinging-to-rechable-host-using-icmpclient-of-poco-l
      // libpoconet62
      for (YAML::detail::iterator_value config_service: config_host["services"])
        services.push_back(config_service.as<string>());

      MonitorHost host = MonitorHost( config_host["label"].as<string>(), 
        config_host["address"].as<string>(), services );

      hosts.push_back(host);
    }

	}	

  // Maybe we can do an eachHost() thing, passing the host and services to the iterator
  // that would be useful in the destructor too
	void printtest() { 
    cout << "To  :" << to << endl << "From:" << from << endl;

    for (MonitorHost host: hosts) {
      cout << "  * host: " << host.label << " - " << host.address << endl;
      cout << "  * Services:" << endl;

      for(string service: host.services)
        cout << "    * " << service << endl;
    }
	} 
}; 


int main(int argc, char* argv[]) {
	if (argc < 2) {
		cerr << "Usage: " << argv[0] << " config.yml" << endl;
		return 1;
	}

  MonitorJob *job;

  try {
    job = new MonitorJob(argv[1]);
    job->printtest();
  } catch(const YAML::Exception& e) {
    cerr << "YAML Exception: " << e.what() << endl;
    return 1;
  } catch(const std::invalid_argument& e) {
    cerr << "invalid_argument Encountered: " << e.what() << endl;
    return 1;
  } catch(const exception& e) {
    cerr << "General Exception Encountered: " << e.what() << endl;
    return 1;
  }

  delete job;

  return 0;
}
