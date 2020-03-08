#include <map>

#define TUPLE_INT_STR std::tuple<unsigned int, std::string>
#define MAP_STR_STR std::map<std::string, std::string>

class WebClient {
  public:
    std::string address;
    unsigned int port;
    bool isSSL;

    WebClient(std::string, int, bool);
    TUPLE_INT_STR post(std::string, std::string, MAP_STR_STR);
    TUPLE_INT_STR get(std::string path, MAP_STR_STR);

    TUPLE_INT_STR post(std::string path, std::string body);
    TUPLE_INT_STR get(std::string path);
	private:
    TUPLE_INT_STR req(std::string, std::string *, std::string *, MAP_STR_STR *);
};

