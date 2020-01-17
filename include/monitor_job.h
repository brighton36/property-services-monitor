#ifndef MONITOR_JOB_H
#define MONITOR_JOB_H

class MonitorHost { 
  public:
  std::string label, address;
  std::vector<std::string> services;
	MonitorHost(std::string label, std::string address, std::vector<std::string> services);
};

class MonitorJob { 
	public: 
	std::string config_path, to, from, smtp_host, subject;
  std::vector<MonitorHost> hosts;
  std::vector<std::vector<std::string>*> host_services;
	MonitorJob(char *szPath);	
	void printtest(); 
}; 

#endif
