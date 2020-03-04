
#include "Poco/Net/HTTPClientSession.h"

class WebClient {
  public:
    std::string address;
    unsigned int port;
    bool isSSL;
    Poco::Net::HTTPClientSession session;

    WebClient(std::string, int, bool);
    std::tuple<unsigned int, std::string> post(std::string, std::string);
    std::tuple<unsigned int, std::string> get(std::string path);
};

