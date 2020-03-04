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

#ifndef PROPERTY_SERVICES_MONITOR_H
#define PROPERTY_SERVICES_MONITOR_H

#ifdef __GNUC__
#define UNUSED __attribute((unused))
#else
#define UNUSED
#endif

#define PTR_MAP_STR_STR std::shared_ptr<std::map<std::string, std::string>>
#define MISSING_FIELD "Missing \"{}\" field."

#include "monitor_service.h"

bool pathIsReadable(std::string);
bool has_any(std::vector<std::string>, std::vector<std::string>);

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

#endif
