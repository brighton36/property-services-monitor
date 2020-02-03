#include <iostream>
#include <regex>
#include <vector>
#include <memory>
#include <unordered_map> 
#include <map> 
#include <fmt/ostream.h>
#include <filesystem>

#include "yaml-cpp/yaml.h"

#include "Poco/Net/HTTPResponse.h"
#include "Poco/Net/MailMessage.h"

#include "inja.hpp"

#ifndef MONITOR_JOB_H
#define MONITOR_JOB_H

#ifdef __GNUC__
#define UNUSED __attribute((unused))
#else
#define UNUSED
#endif

#define PTR_UMAP_STR std::shared_ptr<std::unordered_map<std::string, std::string>>
#define MISSING_FIELD "Missing \"{}\" field."

class MonitorServiceBase { 
  public:
    std::string address, type;
    PTR_UMAP_STR params;
    PTR_UMAP_STR results;

    MonitorServiceBase(std::string type, std::string, PTR_UMAP_STR);

    virtual bool IsAvailable();
};

template<typename T> std::shared_ptr<MonitorServiceBase> \
  createT(std::string address, PTR_UMAP_STR params) {
    return std::make_shared<T>(address, params); }

struct MonitorServiceFactory {
  typedef std::map<std::string, std::shared_ptr<MonitorServiceBase>(*)( \
    std::string address, PTR_UMAP_STR params)> map_type;

  static std::shared_ptr<MonitorServiceBase> createInstance(
    std::string const& s, std::string address, PTR_UMAP_STR params) {

    map_type::iterator it = getMap()->find(s);
    if(it == getMap()->end())
      return 0;
    return it->second(address, params);
  }

  protected:
    static map_type * getMap() {
      if(!map) { map = new map_type; } 
      return map; 
    }

  private:
    // I'm pretty sure this can't be a shared_ptr, and must exist until program
    // termination, since we can't guarantee the destruction order:
    static map_type * map;
};

template<typename T>
struct ServiceRegister : MonitorServiceFactory { 
  ServiceRegister(std::string const& s) { 
    getMap()->insert(std::make_pair(s, &createT<T>));
  }
};

class MonitorServicePing : public MonitorServiceBase { 
  public:
    unsigned int tries, success_over;
    MonitorServicePing(std::string, PTR_UMAP_STR);
    bool IsAvailable();
  private:
    static ServiceRegister<MonitorServicePing> reg;
}; 

class MonitorServiceWeb : public MonitorServiceBase { 
  public:
    unsigned int port, status_equals;
    std::string path;
    std::string ensure_match;
		bool isHttps;

    MonitorServiceWeb(std::string, PTR_UMAP_STR);
    bool IsAvailable();
    std::string HttXRequest(std::string path, Poco::Net::HTTPResponse &response);
  private:
    static ServiceRegister<MonitorServiceWeb> reg;
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
    nlohmann::json ToJson();  
}; 

class NotifierSmtp { 
  public:
    std::string to, from, subject, host, username, password;
    std::string base_path, template_html_path, template_plain_path;
    unsigned int port;
    bool isSSL;
    PTR_UMAP_STR parameters;

    NotifierSmtp(std::string, const YAML::Node);
    NotifierSmtp() {};
    bool SendResults(nlohmann::json*);
    bool DeliverMessage(Poco::Net::MailMessage *message);
  private:
    bool PathIsReadable(std::string);
    std::unique_ptr<inja::Environment> GetInjaEnv();
};

#endif
