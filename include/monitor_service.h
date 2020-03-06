#include "property-services-monitor.h"

class MonitorServiceBase { 
  public:
    std::string address, type;
    PTR_MAP_STR_STR params;

    MonitorServiceBase(std::string, std::string, PTR_MAP_STR_STR);
    virtual RESULT_TUPLE fetchResults();

    template<typename... Args> 
    void err(std::shared_ptr<std::vector<std::string>> errors, std::string reason, Args... args) {
      errors->push_back(fmt::format(reason, args...));
    }
  protected:
    void setParameters(std::map<std::string,std::function<void(std::string)>>);
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
