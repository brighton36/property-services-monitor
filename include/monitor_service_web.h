#include "property-services-monitor.h"

#include "web_client.h"

class MonitorServiceWeb : public MonitorServiceBase { 
  public:
    unsigned int port, status_equals;
    std::string path;
    std::string ensure_match;
    bool isHttps;

    MonitorServiceWeb(std::string, PTR_MAP_STR_STR);
    RESULT_TUPLE fetchResults();
    static std::string Help();
  private:
    std::unique_ptr<WebClient> client;
    static ServiceRegister<MonitorServiceWeb> reg;
}; 

