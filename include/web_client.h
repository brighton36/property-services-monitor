
#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
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
    std::tuple<unsigned int, std::string> post(std::string, std::string);
    std::tuple<unsigned int, std::string> get(std::string path);
};

