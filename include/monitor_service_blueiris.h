#include "property-services-monitor.h"

#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPSClientSession.h"
#include "Poco/StreamCopier.h"
#include "Poco/Exception.h"

class WebClient {
  public:
    std::string address;
    unsigned int port;
    bool isSSL;
    Poco::Net::HTTPClientSession session;

    WebClient(std::string, int, bool);
    std::string post(std::string, std::string);
    std::string get(std::string path);
};

/*
class BlueIrisAlert {
  std::string camera, short_clip, displayed_length, short_path, resolution;
  unsigned int offset;
  time_t occurred_at;
  public:
    BlueIrisAlert(nlohmann::json);
};
*/

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

}; 
