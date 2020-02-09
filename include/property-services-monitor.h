#include <iostream>
#include <regex>
#include <vector>
#include <memory>
#include <map> 
#include <fmt/ostream.h>
#include <filesystem>

#include "yaml-cpp/yaml.h"

#include "Poco/Net/HTTPResponse.h"
#include <Poco/Net/MailMessage.h>
#include <Poco/Net/FilePartSource.h>

#include "inja.hpp"

#ifndef MONITOR_JOB_H
#define MONITOR_JOB_H

#ifdef __GNUC__
#define UNUSED __attribute((unused))
#else
#define UNUSED
#endif

#define PTR_MAP_STR_STR std::shared_ptr<std::map<std::string, std::string>>
#define MISSING_FIELD "Missing \"{}\" field."

bool pathIsReadable(std::string);

class MonitorServiceBase { 
  public:
    std::string address, type;
    PTR_MAP_STR_STR params;
    PTR_MAP_STR_STR results;

    MonitorServiceBase(std::string, std::string, PTR_MAP_STR_STR);

    virtual bool isAvailable();
  protected:
    bool resultAdd(std::string, std::string);
    template<typename... Args> bool resultFail(std::string reason, Args... args) {
      resultAdd("failure_reason", fmt::format(reason, args...));
      return false;
    }
};

template<typename T> std::shared_ptr<MonitorServiceBase> \
  createT(std::string address, PTR_MAP_STR_STR params) {
    return std::make_shared<T>(address, params); }

template<typename T> std::string helpT() { return T::Help(); }

struct MonitorServiceMapEntry{
  public:
    std::shared_ptr<MonitorServiceBase>(*constructor)(std::string address, PTR_MAP_STR_STR params);
    std::string (*help)();
};

struct MonitorServiceFactory {
  typedef std::map<std::string, MonitorServiceMapEntry> map_type;

  public:
    static std::shared_ptr<MonitorServiceBase> createInstance(
      std::string const& s, std::string address, PTR_MAP_STR_STR params) {

      map_type::iterator it = getMap()->find(s);
      if(it == getMap()->end()) return 0;
      return it->second.constructor(address, params);
    }

    static std::string getHelp(std::string const &s) {
      map_type::iterator it = getMap()->find(s);
      if(it == getMap()->end()) return 0;
      return it->second.help();
    }

    static std::vector<std::string> getRegistrations() {
      std::vector<std::string> ret; 
      auto m = MonitorServiceFactory::getMap();
      for(auto it = m->begin(); it != m->end(); it++) ret.push_back(it->first);
      return ret;
    }

  protected:
    static std::shared_ptr<map_type> getMap() {
      if(!map) { map = std::make_shared<map_type>(); } 
      return map; 
    }

  private:
    static std::shared_ptr<map_type> map;
};

template<typename T>
struct ServiceRegister : MonitorServiceFactory { 
  ServiceRegister(std::string const& s) { 
    MonitorServiceMapEntry me;
    me.constructor = &createT<T>;
    me.help = &helpT<T>;

    getMap()->insert(std::make_pair(s, me));
  }
};

class MonitorServicePing : public MonitorServiceBase { 
  public:
    unsigned int tries, success_over;
    MonitorServicePing(std::string, PTR_MAP_STR_STR);
    bool isAvailable();
    static std::string Help();
  private:
    static ServiceRegister<MonitorServicePing> reg;
}; 

class MonitorServiceWeb : public MonitorServiceBase { 
  public:
    unsigned int port, status_equals;
    std::string path;
    std::string ensure_match;
		bool isHttps;

    MonitorServiceWeb(std::string, PTR_MAP_STR_STR);
    bool isAvailable();
    std::string httxRequest(std::string path, Poco::Net::HTTPResponse &response);
    static std::string Help();
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
    nlohmann::json toJson();  
}; 

class SmtpAttachment {
  public:
    std::string full_path, contents_hash, file_mime_type;
    std::unique_ptr<Poco::Net::FilePartSource> file_part_source;
    SmtpAttachment(std::string, std::string);
    SmtpAttachment(const SmtpAttachment &s2);
    bool operator==(const SmtpAttachment &s2);
    std::string getFilename();
    std::string getContentID();
    void attachTo(Poco::Net::MailMessage *m);
  private:
    void setFilepath(std::string);
};

class NotifierSmtp { 
  public:
    std::string to, from, subject, host, username, password, base_path;
    std::string template_subject, template_html_path, template_plain_path;
    unsigned int port;
    bool isSSL;
    PTR_MAP_STR_STR parameters;
    std::vector<SmtpAttachment> attachments;

    NotifierSmtp(std::string, const YAML::Node);
    NotifierSmtp() {};
    bool sendResults(nlohmann::json*);
    bool deliverMessage(Poco::Net::MailMessage *message);
    static std::string Help();
  private:
    nlohmann::json getNow();
    std::string toFullPath(std::string, std::string);
    std::unique_ptr<inja::Environment> getInjaEnv();
};

#endif
