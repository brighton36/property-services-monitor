#include <iostream>
#include <regex>
#include <vector>
#include <memory>
#include <unordered_map> 
#include <map> 
#include <fmt/ostream.h>

#include "Poco/Net/HTTPResponse.h"
#include "Poco/Net/MailMessage.h"

#ifndef MONITOR_JOB_H
#define MONITOR_JOB_H

#ifdef __GNUC__
#define UNUSED __attribute((unused))
#else
#define UNUSED
#endif

#define PTR_UMAP_STR std::shared_ptr<std::unordered_map<std::string, std::string>>

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
    std::string label, address;
    std::vector<std::shared_ptr<MonitorServiceBase>> services;
    MonitorHost(std::string label, std::string, 
      std::vector<std::shared_ptr<MonitorServiceBase>>);
};

class MonitorJob { 
  public: 
    std::string config_path, to, from, subject;
    PTR_UMAP_STR smtp_params;
    std::vector<std::shared_ptr<MonitorHost>> hosts;
    MonitorJob(std::string);  
    MonitorJob(){};  
}; 

class SmtpNotifier { 
  public:
    std::string host, username, password;
    unsigned int port;

    SmtpNotifier(PTR_UMAP_STR);
    SmtpNotifier() {};
    bool Send(Poco::Net::MailMessage *message);
};

#endif
