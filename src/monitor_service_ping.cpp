#include "property-services-monitor.h"

#include "Poco/Net/ICMPClient.h"

using namespace std;

ServiceRegister<MonitorServicePing> MonitorServicePing::reg("ping");

MonitorServicePing::MonitorServicePing(string address, PTR_MAP_STR_STR params) 
  : MonitorServiceBase("ping", address, params) { 

  // Default values:
  tries = 5;
  success_over = 4;

  for( const auto& n : *params )
    if ("tries" == n.first)
      tries = stoi(n.second);
    else if ("success_over" == n.first)
      success_over = stoi(n.second);
    else
      throw invalid_argument(fmt::format("Unrecognized ping parameter \"{}\".", 
        n.first));
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
