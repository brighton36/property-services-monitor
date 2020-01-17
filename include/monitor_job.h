#ifndef MONITOR_JOB_H
#define MONITOR_JOB_H

class MonitorServiceBase { 
	public:
	std::string address, type;

	MonitorServiceBase(std::string address, std::string type);
};

class MonitorServicePing : public MonitorServiceBase { 
	public:
  MonitorServicePing(std::string address);
	bool IsAvailable();
}; 

class MonitorServiceWeb : public MonitorServiceBase { 
	public:
  MonitorServiceWeb(std::string address);
}; 

class MonitorHost { 
  public:
  std::string label, address;
  std::vector<MonitorServiceBase *> services;
	MonitorHost(std::string label, std::string address, std::vector<MonitorServiceBase *> services);
  ~MonitorHost();
};

class MonitorJob { 
	public: 
	std::string config_path, to, from, smtp_host, subject;
  std::vector<MonitorHost *> hosts;
	MonitorJob(char *szPath);	
  ~MonitorJob();
	void printtest(); 
}; 

#endif
