#include "property-services-monitor.h"

class MonitorServiceWeb : public MonitorServiceBase { 
  public:
    unsigned int port, status_equals;
    std::string path;
    std::string ensure_match;
    bool isHttps;

    MonitorServiceWeb(std::string, PTR_MAP_STR_STR);
    bool isAvailable();
    std::string httxRequest(std::string path, Poco::Net::HTTPResponse &);
    static std::string Help();
  private:
    static ServiceRegister<MonitorServiceWeb> reg;
}; 

