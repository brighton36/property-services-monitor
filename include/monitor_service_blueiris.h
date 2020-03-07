#include "property-services-monitor.h"
#include "blue_iris_alert.h"
#include "web_client.h"

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

class MonitorServiceBlueIris : public MonitorServiceBase { 
  public:
    std::string username, password, session;
    std::string capture_from, capture_to, capture_camera;
    unsigned int max_warnings, min_uptime;
    float min_percent_free;

    MonitorServiceBlueIris(std::string, PTR_MAP_STR_STR);
    ~MonitorServiceBlueIris();
    RESULT_TUPLE fetchResults();
    static std::string Help();
  private:
    std::string tmp_dir;
    std::unique_ptr<WebClient> client;
    static ServiceRegister<MonitorServiceBlueIris> reg;
    nlohmann::json sendCommand(std::string, nlohmann::json);
    std::shared_ptr<std::vector<BlueIrisAlert>> getAlertsCommand(time_t, std::string);
    std::shared_ptr<std::vector<std::string>> fetchAlertImages();
    std::string createTempDirectory();
}; 

