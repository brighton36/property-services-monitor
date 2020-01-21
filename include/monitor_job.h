#include <iostream>
#include <vector>
#include <memory>
#include <unordered_map> 
#include <map> 

#ifndef MONITOR_JOB_H
#define MONITOR_JOB_H

#ifdef __GNUC__
#define UNUSED __attribute((unused))
#else
#define UNUSED
#endif

class MonitorServiceBase { 
	public:
    std::string address, type;

    MonitorServiceBase(std::string address, std::string type);
    ~MonitorServiceBase();
    virtual bool IsAvailable();
};


////
// TODO: Switch all this to shared_ptr
template<typename T> MonitorServiceBase * createT(std::string address) {
  return new T(address); }

struct MonitorServiceFactory {
  typedef std::map<std::string, MonitorServiceBase*(*)(std::string address)> map_type;

  static MonitorServiceBase * createInstance(std::string const& s, std::string address) {
    map_type::iterator it = getMap()->find(s);
    if(it == getMap()->end())
      return 0;
    return it->second(address);
  }

  protected:
    static map_type * getMap() {
      // never delete'ed. (exist until program termination)
      // because we can't guarantee correct destruction order 
      if(!map) { map = new map_type; } 
      return map; 
    }

  private:
    static map_type * map;
};

// TODO: Rename DerivedService to ServiceRegister
template<typename T>
struct ServiceRegister : MonitorServiceFactory { 
  ServiceRegister(std::string const& s) { 
    getMap()->insert(std::make_pair(s, &createT<T>));
  }
};

////

class MonitorServicePing : public MonitorServiceBase { 
	public:
    MonitorServicePing(std::string address);
    ~MonitorServicePing();
    bool IsAvailable();
    std::unordered_map<std::string, std::string> Results();
	private:
		static ServiceRegister<MonitorServicePing> reg;
}; 

class MonitorServiceWeb : public MonitorServiceBase { 
	public:
    MonitorServiceWeb(std::string address);
    ~MonitorServiceWeb();
	private:
		static ServiceRegister<MonitorServiceWeb> reg;
}; 

class MonitorHost { 
  public:
    std::string label, address;
    std::vector<std::shared_ptr<MonitorServiceBase>> services;
    MonitorHost(std::string label, std::string address, 
      std::vector<std::shared_ptr<MonitorServiceBase>> services);
    ~MonitorHost();
};

class MonitorJob { 
	public: 
    std::string config_path, to, from, smtp_host, subject;
    std::vector<std::shared_ptr<MonitorHost>> hosts;
    MonitorJob(std::string path);	
    ~MonitorJob();
    void printtest(); 
}; 

#endif
