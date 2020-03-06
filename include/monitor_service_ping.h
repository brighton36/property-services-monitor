#include "property-services-monitor.h"

class MonitorServicePing : public MonitorServiceBase { 
  public:
    unsigned int tries, success_over;
    MonitorServicePing(std::string, PTR_MAP_STR_STR);
    RESULT_TUPLE fetchResults();
    static std::string Help();
  private:
    static ServiceRegister<MonitorServicePing> reg;
}; 
