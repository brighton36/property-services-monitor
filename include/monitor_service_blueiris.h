#include "property-services-monitor.h"

#include "web_client.h"

#include <ctime>

class BlueIrisAlert {
  public:
    std::string camera, clip, filesize, short_path, resolution, path, res;
    unsigned int color, flags, newalerts, offset, zones;
    time_t date;

    BlueIrisAlert(nlohmann::json);
    std::string dateAsString(std::string);
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

BlueIrisAlert::BlueIrisAlert(nlohmann::json from) {
	camera    = from["camera"];
	clip      = from["clip"];
	color     = from["color"];
	filesize  = from["filesize"];
	flags     = from["flags"];
  newalerts = from.value("newalerts", 0);
	offset    = from["offset"];
	path      = from["path"];
	res       = from["res"];
	zones     = from["zones"];

  // NOTE: I'm ignoring zones here. Which, seems to work as expected as long as
  // "we" are in the same zone as the server we're querying. 
  date = static_cast<time_t>(from["date"]);
}

std::string BlueIrisAlert::dateAsString(std::string fmt) {
  char strf_out[80];

  strftime(strf_out,80,fmt.c_str(),localtime(&date));

  return std::string(strf_out);
}

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
