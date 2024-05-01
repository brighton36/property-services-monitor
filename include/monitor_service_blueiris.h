#include "property-services-monitor.h"

class BlueIrisAlert {
  public:
    std::string camera, clip, filesize, resolution, path, res;
    unsigned int color, flags, newalerts, offset, zones;
    time_t date;

    BlueIrisAlert(nlohmann::json);
    std::string dateAsString(std::string);
    std::string pathThumb();
    std::string pathClip(unsigned int);
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

class MonitorServiceBlueIris : public MonitorServiceBase { 
  public:
    std::string username, password, session;
    std::string capture_from, capture_to, capture_camera;
    unsigned int max_warnings, min_uptime, ignore_warnings;
    float min_percent_free;

    MonitorServiceBlueIris(std::string, PTR_MAP_STR_STR);
    ~MonitorServiceBlueIris();
    RESULT_TUPLE fetchResults();
    static std::string Help();
    unsigned int static uptime_to_seconds(std::string);
  private:
    std::string tmp_dir;
    std::unique_ptr<WebClient> client;
    static ServiceRegister<MonitorServiceBlueIris> reg;
    nlohmann::json sendCommand(std::string, nlohmann::json);
    std::shared_ptr<std::vector<BlueIrisAlert>> getAlertsCommand(time_t, std::string);
    nlohmann::json fetchAlertImages();
    nlohmann::json fetchImage(BlueIrisAlert &, std::string);
    std::string createTempDirectory();
    std::tuple<unsigned int, unsigned int> imageDimensions(const char *, unsigned int);
}; 

