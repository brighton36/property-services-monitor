#include <map>

#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPRequest.h"

#define TUPLE_INT_STR std::tuple<unsigned int, std::string>
#define MAP_STR_STR std::map<std::string, std::string>

class WebClient {
  public:
    std::string address;
    unsigned int port;
    bool isSSL;
    Poco::Net::HTTPClientSession session;

    WebClient(std::string, int, bool);
    TUPLE_INT_STR post(std::string, std::string, MAP_STR_STR);
    TUPLE_INT_STR get(std::string path, MAP_STR_STR);

    TUPLE_INT_STR post(std::string path, std::string body) {
      return post(path, body, MAP_STR_STR()); 
    }
    TUPLE_INT_STR get(std::string path) { 
      return get(path, MAP_STR_STR()); 
    }
	private:
    TUPLE_INT_STR req(Poco::Net::HTTPRequest *, std::string, MAP_STR_STR);
};

