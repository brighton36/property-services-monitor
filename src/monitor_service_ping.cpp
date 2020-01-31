#include "monitor_job.h"

#include "Poco/Net/ICMPClient.h"

using namespace std;

ServiceRegister<MonitorServicePing> MonitorServicePing::reg("ping");

MonitorServicePing::MonitorServicePing(string address, PTR_UMAP_STR params) 
  : MonitorServiceBase("ping", address, params) { 

  // Default values:
  this->tries = 5;
  this->success_over = 4;

  for( const auto& n : *params )
    if ("tries" == n.first)
      this->tries = stoi(n.second);
    else if ("success_over" == n.first)
      this->success_over = stoi(n.second);
    else
      throw invalid_argument(fmt::format("Unrecognized ping parameter \"{}\".", 
        n.first));
}

bool MonitorServicePing::IsAvailable() {
  MonitorServiceBase::IsAvailable();

  try {
    // The port does not matter at all. Not sure why the library makes us set it.
    auto socketAddress = Poco::Net::SocketAddress(this->address, "80");

    unsigned int successful_pings = Poco::Net::ICMPClient::ping(
      socketAddress, Poco::Net::IPAddress::IPv4, this->tries);

    this->results->emplace("successful_pings", to_string(successful_pings));

    if (successful_pings > this->success_over) 
      return true;
    else {
      // TODO: maybe we should have a fail() function that does this and returns false
      this->results->emplace("failure_reason", 
        fmt::format("{} out of the necessary {} pings received.", successful_pings,
          this->success_over));
      return false;
    }

  } catch(const Poco::IOException& e) {
    this->results->emplace("failure_reason", e.what());
  }

  return false;
}
