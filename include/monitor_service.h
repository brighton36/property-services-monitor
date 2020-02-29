#include "property-services-monitor.h"

class MonitorServiceBase { 
  public:
    std::string address, type;
    PTR_MAP_STR_STR params;
    PTR_MAP_STR_STR results;

    MonitorServiceBase(std::string, std::string, PTR_MAP_STR_STR);

    virtual bool isAvailable();
  protected:
    void setParameters(std::map<std::string,std::function<void(std::string)>>);
    bool resultAdd(std::string, std::string);
    template<typename... Args> bool resultFail(std::string reason, Args... args) {
      resultAdd("failure_reason", fmt::format(reason, args...));
      return false;
    }
};

class MonitorHost { 
  public:
    std::string label, description, address;
    std::vector<std::shared_ptr<MonitorServiceBase>> services;
    MonitorHost(std::string, std::string, std::string,
      std::vector<std::shared_ptr<MonitorServiceBase>>);
};

class MonitorJob { 
  public: 
    std::vector<std::shared_ptr<MonitorHost>> hosts;
    MonitorJob(const YAML::Node);  
    MonitorJob(){};  
    nlohmann::json toJson();  
}; 
