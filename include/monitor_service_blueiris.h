#include "property-services-monitor.h"

#include "web_client.h"

#include <ctime>

class BlueIrisAlert {
  public:
    std::string camera, clip, filesize, resolution, path, res;
    unsigned int color, flags, newalerts, offset, zones;
    time_t date;

    BlueIrisAlert(nlohmann::json);
    std::string dateAsString(std::string);
    std::string pathThumb();
    std::string pathClip();
};

class MonitorServiceBlueIris : public MonitorServiceBase { 
  public:
    std::string username, password, session;
    MonitorServiceBlueIris(std::string, PTR_MAP_STR_STR);
    bool isAvailable();
    static std::string Help();
  private:
    std::unique_ptr<WebClient> client;
    static ServiceRegister<MonitorServiceBlueIris> reg;
    nlohmann::json sendCommand(std::string, nlohmann::json);
    std::shared_ptr<std::vector<BlueIrisAlert>> getAlerts(time_t, std::string);
}; 


class BlueIrisException : public std::exception {
  public:
    std::string s;
    BlueIrisException(std::string ss) : s(ss) {}
    template<typename... Args> BlueIrisException(std::string reason, Args... args) {
      s = fmt::format(reason, args...);
    }
    ~BlueIrisException() throw () {}
    const char* what() const throw() { return s.c_str(); }
};
