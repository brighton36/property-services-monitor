#include "monitor_service_ping.h"

#include "Poco/Net/ICMPClient.h"

using namespace std;

ServiceRegister<MonitorServicePing> MonitorServicePing::reg("ping");

std::string MonitorServicePing::Help() { 
  return 
    " * tries          (optional) The number of pings to attempt. Defaults to 5.\n"
    " * success_over   (optional) The number of pings, over which we are successful. Defaults to 4.\n";
}

MonitorServicePing::MonitorServicePing(string address, PTR_MAP_STR_STR params) 
  : MonitorServiceBase("ping", address, params) { 

  tries = 5;
  success_over = 4;
  
  setParameters({
    {"tries",        [&](string v) { tries = stoi(v);}},
    {"success_over", [&](string v) { success_over = stoi(v);} }
  });
}

bool MonitorServicePing::isAvailable() {
  MonitorServiceBase::isAvailable();

  try {
    // The port does not matter at all. Not sure why the library makes us set it.
    auto socketAddress = Poco::Net::SocketAddress(address, "80");

    unsigned int successful_pings = Poco::Net::ICMPClient::ping(
      socketAddress, Poco::Net::IPAddress::IPv4, tries);

    resultAdd("successful_pings", to_string(successful_pings));

    if (successful_pings > success_over) 
      return true;
    else
      return resultFail("{} out of the necessary {} pings received.", 
        successful_pings, success_over);

  } catch(const Poco::IOException& e) {
    return resultFail(e.what());
  }

  return false;
}
