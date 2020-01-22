#include "monitor_job.h"

#include "Poco/Net/ICMPClient.h"

using namespace std;

ServiceRegister<MonitorServicePing> MonitorServicePing::reg("ping");

MonitorServicePing::MonitorServicePing(string address, PTR_UMAP_STR params) 
  : MonitorServiceBase("ping", address, params) { 
}

PTR_UMAP_STR MonitorServicePing::Results() {
  PTR_UMAP_STR results;

  return results;
}

bool MonitorServicePing::IsAvailable() {
  // TODO: Cache result?
  
  cout << "Ping :" ;

	try {
		const auto attempts = 5;

		auto socketAddress = Poco::Net::SocketAddress("127.0.0.1", "80");
    //socketAddress = Poco::Net::SocketAddress("192.168.1.127", "80");

		auto successful_pings = Poco::Net::ICMPClient::ping(
      socketAddress, Poco::Net::IPAddress::IPv4, attempts);

    if (successful_pings == attempts)
      cout << "OK!" << endl;
    else
      cout << "FAIL" << endl;

	} catch(const Poco::IOException& e) {
		cout << "Must run as root. TODO: Bubble it up " << endl;
	}

  return false;
}
